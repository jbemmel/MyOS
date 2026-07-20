/***************************************************************************
                          DP8390d.cpp  -  description
                             -------------------
    begin                : Tue Apr 30 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/

#include "DP8390d.h"

#include "IVirtualMemory.h"
#include "NE2000Driver.h"
#include "Networking/Ethernet/EthernetPacket.h"

// Only used for debugging (DPRINTK [time] )
#include "TimerFacility/TimeUtil.h"

namespace MyOS { namespace Devices { namespace Network { namespace NE2000 {

	using Drivers::Network::NE2000::NE2000Driver;
	// TODO: using Drivers::Network::NE2000::operator new[];
	
	using namespace Networking::Ethernet;
	using Networking::Ethernet::EthernetPacket;
	using namespace InterruptHandling;

// Chip has 64K of memory, divided in parts of 256 byte each for RX and TX
// Bochs: First 16K are init data
// This can be tweaked
enum MEMPAGES {
   E_TX_START  = 0x40,	// see e.g. ne2.c or any other one
   E_RX_START  = E_TX_START+6*DP8390D::E_TX_BUFFERS,  // n*6*256 byte TX region
   E_PAGE_STOP = 0x80,                                // last valid 256-byte index

   // amount of memory to reserve for rx buffer, could be done otherwise
   E_RX_BUFSIZE_LOG2 = 5,
   E_RX_BUFSIZE		 = (1 << E_RX_BUFSIZE_LOG2) * _4KB		// 32KB
};

/// Bits in command register
enum COMMANDBITS {

   STOP  = (1<<0),
   START = (1<<1),
   TX    = (1<<2),   // starts transmission of a packet, after txcount&adr have been set

   REMOTE_READ    = (1<<3),
   REMOTE_WRITE   = (1<<4),
   SEND_PACKET    = (1<<3)|(1<<4),
   ABORT_DMA      = (1<<5),

   PS0 = 0,
   PS1 = (1<<6),
   PS2 = (1<<7),
   // PS3 = (1<<6)|(1<<7), invalid
   PAGEBITS = PS0 | PS1 | PS2,   // mask
};

/// Bits in ISR register, IMR mask matches
enum ISRBITS {
   RX_OK             = (1<<0),
   TX_OK             = (1<<1),
   RX_ERROR          = (1<<2),
   TX_ERROR          = (1<<3),
   OVERWRITEWARN     = (1<<4),   // localDMA out of buffer space, about to overwrite
   COUNTEROVERFLOW   = (1<<5),   // statistics counter overflow
   RDMA_COMPLETE     = (1<<6),

   // Note: this does not include RDMA_COMPLETE!	
   ALLIRQ = RX_OK|TX_OK|RX_ERROR|TX_ERROR|OVERWRITEWARN|COUNTEROVERFLOW,

   /// Status bit, no INT & writing has no effect? (linux: reset_completed)
   RESET_STATUS      = (1<<7),   // set when reset, cleared at 'start', also set at RxBufferOverflow

   /// writing 0xff clears
   CLEAR = 0xff,
};

enum DATACONFIGBITS {
   DMA_WORD          = (1<<0),
   BIG_ENDIAN        = (1<<1),
   DMA_32BIT         = (1<<2),   // not supported by NE2000
   OPERATIONAL       = (1<<3),   // 0 for loopback
   AUTO_INIT_REMOTE  = (1<<4),   // not supported by NE2000

   // FIFO tresholds in bytes
   FIFO_TH_2         = (0<<5),
   FIFO_TH_4         = (1<<5),
   FIFO_TH_8         = (1<<6),
   FIFO_TH_12        = (1<<5)|(1<<6),
};

enum TXCONFIGBITS {
   DISABLE_CRC          = (1<<0),
   NORMAL_OP            = (0<<1),
   INT_LOOPBACK         = (1<<1),
   EXT_LOOPBACK_LPBK    = (1<<2),   // bit LPBK is set to 1 (whatever that is)
   EXT_LOOPBACK         = (1<<1)|(1<<2),
   ALLOW_REMOTE_DISABLE = (1<<3),   // react to special multicast packets that disable the NIC
   LOW_PRIORITY_NODE    = (1<<4),   // if set, a different backoff algorithm is used (with less chance)
};

enum RXCONFIGBITS {
   ACCEPT_ERROR_PACKETS = (1<<0),
   ACCEPT_RUNT_PACKETS  = (1<<1),   // Accept packets <64 bytes
   ACCEPT_BROADCAST     = (1<<2),
   ACCEPT_MULTICAST     = (1<<3),
   PROMISCUOUS          = (1<<4),
   MONITOR_ONLY         = (1<<5),
};

enum TXSTATUSBITS {
   E_TX_SUCCESS         = (1<<0),
   E_TX_NODEFER         = (1<<1),   // from linux, not in docs
   E_TX_COLLISSION      = (1<<2),   // at least 1 collision, tx rescheduled
   E_TX_ABORTED         = (1<<3),
   E_TX_CARRIERLOST     = (1<<4),
   E_TX_FIFOUNDERRUN    = (1<<5),
   E_TX_CDHEARTBEAT     = (1<<6),
   E_TX_OOWCOLLISSION   = (1<<7),   // out-of-window, tx rescheduled
};

DP8390D::DP8390D( MyOS::Core::IComponent& c )
: IEthernetDevice( c, VERSION(1,0) ), packetProcessor(0) /* was likely already 0 */
, status(E_UNDEFINED)
{
	memset_aligned( pingpong, 0, sizeof(pingpong) );	 // Should already be 0
	for (u32 b=0; b<E_TX_BUFFERS; ++b) {
		pingpong[b].page = E_TX_START + 6*b;
	}
}

/*
DP8390D::~DP8390D(){
}
*/

// virtual 
myos_result_t 
DP8390D::open( IPacketProcessor& rx, u32 mode )
{
	DPRINTK( "\nDP8390D::open" );
	if (packetProcessor==0) {
	    packetProcessor = &rx;
		
		// start the chip now
		txing = inInterrupt = false;
		isr = CLEAR;
		imr = ALLIRQ;  // except remote-DMA-complete (i.e. after sending/receiving a packet)
		commandRegister = START | ABORT_DMA | PS0;
		txConfig = NORMAL_OP;
		   
		// RUNT not linux, but ARP=42 bytes and RUNT<64
		rxConfig = ACCEPT_BROADCAST/*|ACCEPT_MULTICAST*/|ACCEPT_RUNT_PACKETS;   

		// todo: set multicast list (needs PS1)

		// register IRQ handler, Bochs hardcoded to IRQ10 (TODO: Postpone until open() ? )
		NE2000Driver::getInstance().iInterruptHandling->setIRQHandler( E_IRQ10, *this, E_NONE );
		// TODO if fails, stop chip again!
		
		return E_MYOS_SUCCESS;	
	}
	DPRINTK( "->failed!" );
	return E_MYOS_E_FAIL;
}

void
DP8390D::close()
{
	// deinit() ?
	if (packetProcessor) {
	    commandRegister = STOP | ABORT_DMA | PS0;
	    imr = 0;
	    NE2000Driver::getInstance().iInterruptHandling->removeIRQHandler( E_IRQ10 );
	    packetProcessor = 0;	
	}
}

/* virtual
EthernetPacket*
DP8390D::createPacket( size_t maxPayload ) const
{
   // PRINTF( "\ncreatePacket payload=%u", maxPayload );

   // store size in 16bit, return misaligned packet (but aligned IP header)
   if (maxPayload <= EthernetPacket::MTU) {
      maxPayload += sizeof(EthernetPacket)+sizeof(u16);

      // need DMA memory for TX'ing (->also Rx in case of packet reuse)
      u16* mem = (u16*) NE2000Component::getInstance().allocate( maxPayload, E_KM_DMA );

      // store size
      if (mem) {
         *mem++ = (u16) maxPayload;
         return (EthernetPacket*) (mem);   // misaligned, but IP header is aligned
      }
   }
   return 0;   // failed
}
*/

/* virtual
void
DP8390D::releasePacket( EthernetPacket &p ) const
{
   ASSERTIONv( ((u32) &p) > 0xFF800000, Debug::AS_FATAL, (u32) &p );

   u16* size = ((u16*) &p)-1;
   // PRINTF( "\nreleasePacket %x size=%u", size, *size );
   NE2000Component::getInstance().free( size, *size, E_KM_DMA );
}
*/


inline void DP8390D::selectPage( u8 page ) {
   // Preserve all other bits
   commandRegister = (commandRegister & ~PAGEBITS) | page;
}

bool  // find the chip, borrowed from linux' ne.c
DP8390D::probe( /* u16 baseadr */ )
{
   if (commandRegister == 0xff) return false;
   commandRegister = STOP | ABORT_DMA | PS0;

   // clear frame alignment error counter by reading, then test it
   u8 dummy UNUSED = faeCount;
   return faeCount==0;
}

bool
DP8390D::init( /* params ? */ ) // from DP8390D.pdf page 29
{
	if (!probe()) {   // this selects PS0
		status = E_NO_CHIP;
		PRINTK( "\nNE2000: No chip found! Using BASEADDRESS: %x", BASEADDRESS );
		return false;
	}

	// Try a reset first, using bochs soft restart network does not work anymore
	// Bochs triggers on read, not write, and treats this different from a hw reset
	resetport = resetport;  
	
	// allocate 32K rxBuffer here for convenience (no alloc in Rx IRQ)
	// TODO: rxBuffer = new u8[ E_RX_BUFSIZE ];
	//rxBuffer = (u8*) NE2000Driver::getInstance().iVirtualMemory->allocate( E_RX_BUFSIZE_LOG2 );
	//rxCurPacket = rxBuffer+2;  // align on odd 2-byte

	// from linux 8390.c, similar to PDF
	commandRegister = STOP | ABORT_DMA | PS0;
	dataConfig = DMA_WORD | OPERATIONAL | FIFO_TH_8;   // use 16-bit access
	remoteCount0 = 6*2;	// to read MAC address
	remoteCount1 = 0;
	rxConfig = MONITOR_ONLY;
	txConfig = INT_LOOPBACK;   // supported by bochs ?

	// tx page & rx ring
	txPageStart = E_TX_START;
	pageStart   = E_RX_START;       // 12 tx pages, then come rx pages
	boundaryPtr = E_PAGE_STOP - 1;	// one before stop page
	pageStop    = E_PAGE_STOP;

	// clear pending IRQ & mask
	isr = CLEAR;
	imr = 0;

	// read MAC address from the chip's ROM
	for (int b=0; b<6; ++b) {
		// localAddress[b] = dataport_u8;	// read bytes, not words (Bochs complains here)
		localAddress[b] = dataport & 0xff;
	}

	commandRegister = STOP | ABORT_DMA | PS1;

	// set the device's physical address, it gets cleared upon reset!
	physAdr0 = localAddress[0];
	physAdr1 = localAddress[1];
	physAdr2 = localAddress[2];
	physAdr3 = localAddress[3];
	physAdr4 = localAddress[4];
	physAdr5 = localAddress[5];

	curPage = E_RX_START;    // == rx start page
	commandRegister = STOP | ABORT_DMA | PS0;

	status = E_INITIALIZED;
	PRINTK( "\nNE2000 initialized adr=%X:%X:%X:%X:%X:%X",
		localAddress[0],localAddress[1],localAddress[2],
		localAddress[3],localAddress[4],localAddress[5] );
	return true;
}

void
DP8390D::deinit( )
{
   // stop chip
   if (status!=E_NO_CHIP) {
      commandRegister = STOP | ABORT_DMA | PS0;
      rxConfig = 0;
   }

   /* free memory
   if (rxBuffer) {
      NE2000Driver::getInstance().iVirtualMemory->free( rxBuffer, E_RX_BUFSIZE_LOG2 );
      rxBuffer=0;
   }*/
}

void
DP8390D::setPromiscuous()
{
   rxConfig = ACCEPT_BROADCAST | ACCEPT_MULTICAST | PROMISCUOUS | ACCEPT_RUNT_PACKETS;

   // docs: multicast addresses must be set to all 1's
   selectPage(PS1);
   multicastAdr0 = 0xff;
   multicastAdr1 = 0xff;
   multicastAdr2 = 0xff;
   multicastAdr3 = 0xff;
   multicastAdr4 = 0xff;
   multicastAdr5 = 0xff;
   multicastAdr6 = 0xff;
   multicastAdr7 = 0xff;
   selectPage(PS0);
}

/** virtual
 *
 * Tx strategy is as follows: enqueue and if (triggers!=txIrqs) assume a TX irq
 * will follow that will do the transmission. Else, generate a IRQ9 trigger here
 * which does the transmission
 * 
 * Note: 8390.c disables interrupts during this function
 */
myos_result_t
DP8390D::sendPacket( const Networking::Packet& packet, u32 flags )
{
	u32 octets = packet.getTxSize();
	EthernetPacket& pkt = packet.getDataAs<EthernetPacket>();

	// if (status!=E_OPENED) return DEVICE_NOT_OPEN;

	//DPRINTK( "\n[%d] NE2000::transmit packet=%x octets=%u", 
	//	(u32) Timer::TimeUtil::now( Timer::ms ), pkt, octets );
	// pkt->debugDump( octets );

	// verify octets <= max
	if (octets>EthernetPacket::MTU) {
		DPRINTK( " -> #ERROR# packet too big!" );
		return E_MYOS_EMSGSIZE;
	}	// TODO: if < minimum, add padding
	octets += sizeof(EthernetPacket);

   // fill in local address
   /*if ((flags & E_DONT_SET_SENDER)==0)*/ pkt.setLocalAddress( localAddress );

   // PRINTF( "\n[NE2000 tx trig=%d irq=%d {%d,%d}]", txTriggered, txIrq, pingpong[0].bytes, pingpong[1].bytes );

   // disable ints for the ethernet card, see linux driver
   imr = 0;


   // if (inInterrupt) {
   //   PRINTF( "\n### WARNING: Tx while in ISR !" );
   //}

   // XX start at txTriggered%bufcount
	bool result = false;
	for (u8 b=0; b<E_TX_BUFFERS; ++b) {
		JOB* job = &pingpong[b];
		if (job->bytes == 0) {
			job->bytes  = octets;
	        job->packet = &packet;
	        //job->notify = onDone;
	        //job->act 	= act;
	        this->blockOutput( (u16*) packet.getFirstByte(), octets, job->page );

			if (!txing) {
            	job->bytes = -octets; // mark it processed
            	this->triggerTx( octets, job->page );
         	} // else DPRINTK( "ne2k: postponed" );	// else handleTX() will take care of it
         	result = true;
			break;
      	}
   }

   if (!result) {
      DPRINTK( "\n## NE2000 ERROR: No tx buffer available! trig = %d irq = %d", txTriggered, txIrq );
   } else {
      // takes too much time
      // PRINTF( "\n[NE2000 tx buf=%d]", buffer );
   }

//    Doesn't work
//    PRINTF( "Busy-waiting..." );
//    u32 irqs = Processor::interruptsEnabled();
//    Processor::allowInterrupts( true );
//    imr = ALLIRQ;
//    while (pingpong[0].bytes && pingpong[1].bytes);
//    imr = 0;
//    Processor::allowInterrupts( irqs );
//    buffer = pingpong[0].bytes == 0 ? 0 : 1;
//    PRINTF( "done" );
//
   imr = ALLIRQ;

   // could free packet here already...
   return result ? E_MYOS_SUCCESS : E_MYOS_E_FAIL;
}


void
DP8390D::onInterrupt( u32 message, cpu_id_t cpu ) throw() // see 8390.c
{
   inInterrupt = true;

   commandRegister = ABORT_DMA | PS0;  // for IRQ status, JVB: added 'START' here, ok?
   u32 status;
   while ((status = isr) != 0) {
      // DPRINTK( "\nIRQ IRQena=%b status=", Processor::interruptsEnabled(), status );

      if (status&(RX_OK|RX_ERROR)) handleRX();

      if (status&TX_OK) {
         handleTX();
      } else if (status&TX_ERROR) {
         handleTXError();
      }

      if (status&COUNTEROVERFLOW) {
         frameErrors   += faeCount;
         crcErrors     += crceCount;
         missedPackets += missCount;
         isr = COUNTEROVERFLOW;  // ack it
      }

      // switch back to page 0 and START, subroutines restore it now
      // XXX What if a parallel send() operation occurs, possible?
      // commandRegister = START | ABORT_DMA | PS0;
   }

   inInterrupt = false;
}

/**
 * Pre-conditions:
 * - 1 or more (!) packets are ready to be read, device is on page 0
 * - curPage points to the first page for the next packet (updated by device, page == 256 bytes)
 * - boundaryPtr points to the last page read (updated by driver)
 *
 * Operation: read all packets while boundaryPtr != curPage, finally ACK IRQ 
 */
void
DP8390D::handleRX()
{
    // read from page 0
    u8 thisframe = boundaryPtr + 1;     // one frame behind
    do {
        // switch to page 1
    	commandRegister = ABORT_DMA | PS1;
    	u8 rxpage = curPage;       // may change while we're processing
    	
    	if (thisframe>=E_PAGE_STOP) thisframe=E_RX_START;   // wrap to rx_start
    
    	if (rxpage!=thisframe) {
    		u16 curOffset = thisframe<<8;
    
    		// ugly io string reads, read packet header to determine size
    		struct PacketHeader {
    			u8 status;
    			u8 nextptr;
    			u16 size;   // includes size of this header
    		} PACKED header;
    
    		enum E_PKTSTATUS {
    			E_RXOK       = (1<<0),
    			E_MULTICAST  = (1<<5),   // multi or broadcast
    		};
    
    		// do DMA, header starts on page boundary
    		blockInput( (u16*) &header, sizeof(header), curOffset );
    		
    		register size_t framesize = header.size - sizeof(header);     // read once    
    		DPRINTK( "\n[%d] eth0 rxIRQ frame=%x status=%X framesize=%u",
    			(u32) Timer::TimeUtil::now(Timer::ms), thisframe, 
    			header.status, framesize );
    
    		// ACK interrupt? it's masked anyway, and we ACK once below 
    		// isr = RDMA_COMPLETE;
    
    		// read the packet, verify that size makes sense, could be 1 AND operation
    		if ((header.status & 0xF) == E_RXOK 
    			&& (framesize <= EthernetPacket::MAXFRAMESIZE)) {

                // go to next packet, assuming not valid when header.status is not?
                thisframe = header.nextptr;                 
    		        		    
    			/* for now, wrap & simply overwrite without warning
    			if (rxCurPacket+framesize > rxBuffer+E_RX_BUFSIZE) {
    				rxCurPacket = rxBuffer+2; // wrap, align on odd 2-byte boundary
    			}*/
                
                Packet &rx = packetProcessor->allocatePacket( framesize );

    			blockInput( (u16*) rx.getData(), framesize, curOffset + sizeof(header) );
    			++rxCount;
    
    			// buffer packet( rxCurPacket, framesize-sizeof(EthernetPacket) );
    			u32 packetFlags = (header.status & E_MULTICAST) 
    				? MULTICAST_MATCH 
    				: PHYSICAL_MATCH;
    
    			// always add multiple of 4 to remain odd-aligned
    			// rxCurPacket += framesize + (4-(framesize&3)); // otherwise overwrite packets
    			
    			// call client (cannot be null), XXX perhaps better to buffer first?
    			packetProcessor->onPacket( rx, packetFlags );
   			    
    		} else {
    		    DPRINTK( "Invalid header status/framesize, aborting receive loop" );
    		    break;
    		}    
    	} else {
    		break;    // done reading
    	}
    } while (1);
	
    // Back to page 0
	commandRegister = ABORT_DMA | PS0;
	
    // ACK RX IRQ
    isr = RX_OK | RX_ERROR | RDMA_COMPLETE;

    // update boundary ptr (only once? or for each packet?)   
	boundaryPtr = (thisframe!=E_RX_START) ? thisframe - 1 : E_PAGE_STOP;
}

void
DP8390D::handleTX()
{
	// u32 status = txStatus;
	isr = TX_OK; // ack it first
	++txIrq;

//   PRINTF( "\n[%d] eth0 txIRQ trig=%d irq=%d", Timing::TSC::read(), txTriggered, txIrq );

	// start from 0 ?
	for (u8 buf=0; buf<E_TX_BUFFERS; ++buf) {
		JOB* job = &pingpong[buf];
		s16 length = job->bytes;
		if ( length < 0 ) {		// i.e. is in buffer (blockoutput)
			// IPacketSender* notify = job->notify;   // make local copy
			job->bytes = 0;	// free buffer, do this after notify?

			JOB* next = &pingpong[(buf+1)%E_TX_BUFFERS];
			s16 bytes = next->bytes;
			if ( bytes > 0 ) {
            	next->bytes = -bytes;	// mark it triggered
            	this->triggerTx( bytes, next->page );
         	} else txing = false;	// now sendPacket() can trigger again

         	// after triggering next one, XX buffer already marked free...
         	// Client should not refer to contents of packet anymore!
         	//if (notify) {
         	//	notify->onPacketSent( buffer( (u8*) job->packet, -length ), job->act );
         	//}
			
			Networking::Packet* p = (Networking::Packet*) job->packet;
			job->packet = 0;
			packetProcessor->onPacketSent( *p );
			
         	return;
		}
   }

   DPRINTK( " : ERROR, unexpected txdone" );
   return;
}

void
DP8390D::handleTXError()
{
   u8 status = txStatus;
   isr = TX_ERROR;    // ack it

   DPRINTK( "\n[NE2000 TX error]" );

   // check if it was aborted, if yes then process
   if (status & (E_TX_ABORTED|E_TX_FIFOUNDERRUN) ) {
      handleTX();
   }
}

void
DP8390D::triggerTx( size_t bytes, u8 startPage )
{
	//DPRINTK( "\n[%d] NE2000::triggerTx octets=%u page=%X", 
	//	(u32) Timer::TimeUtil::now( Timer::ms ), bytes, startPage );
		
	// see linux 8390.c
	commandRegister = ABORT_DMA | PS0;
	if ((commandRegister & TX)==0) {
		txCount0 = bytes & 0xff;
		txCount1 = bytes >> 8;
		txPageStart = startPage;
		txing = true;
		commandRegister = ABORT_DMA | TX | START;  // could also use SEND_PACKET ?
		++txTriggered;
	} else {
		// error: called while tx'ing
		BUG( "\ntriggerTx called while TX'ing" );
	}
}

/**
 * Reads bytes from the device to CPU memory
 */
void
DP8390D::blockInput( u16* buffer, size_t bytes, size_t offset )
{
   // DPRINTK( "\nblockInput buf=%x bytes=%u ofs=%x", buffer, bytes, offset );

   // always read in words, read 1 byte too many if needed
   bytes += (bytes&1);

   // prepare DMA for reading
   commandRegister = START | ABORT_DMA | PS0;
   remoteCount0 = bytes & 0xff;
   remoteCount1 = bytes >> 8;
   remoteStart0 = offset & 0xff;
   remoteStart1 = offset >> 8;
   commandRegister = START | REMOTE_READ | PS0;

   // do the read with insw
   dataport.readString( buffer, bytes );

   // ACK interrupt, now done once
   // isr = RDMA_COMPLETE;

//   for (u32 i=0; i<bytes/2; ++i) {
//       DPRINTK(" %X %X", buffer[i]>>8, buffer[i]&0xff );
//   }
}

/**
 * Sends bytes from the CPU to the chip's paged buffers
 * see ne2.c block_output
 */
void
DP8390D::blockOutput( u16* buffer, size_t bytes, u8 page )
{
   // PRINTF( "\nblockOutput buf=%x bytes=%u page=%X", buffer, bytes, page );

   // round up count
   bytes += (bytes&1);

   // isr = RDMA_COMPLETE; ?? ne.c does this

   // prepare DMA for writing
   commandRegister = START | ABORT_DMA | PS0;   // maybe not needed
   remoteCount0 = bytes & 0xff;
   remoteCount1 = bytes >> 8;
   remoteStart0 = 0;    // page aligned
   remoteStart1 = page;
   commandRegister = START | REMOTE_WRITE;

   //for (u32 i=0; i<bytes/2; ++i) {
   //   PRINTF(" %X %X", buffer[i]>>8, buffer[i]&0xff );
   //}

   // do the write with outsw
   dataport.writeString( buffer, bytes );

   // wait until done, linux checks for 20ms timeout here
   // (not in ISR mask, so no IRQ is triggered)
   while ((isr & RDMA_COMPLETE) == 0);

   // ACK interrupt
   isr = RDMA_COMPLETE;
}

// virtual 
myos_result_t
DP8390D::getAddress( IO::OStream& out ) const {
	for (int c=0; c<6; ++c) out.printf( "%X", localAddress[c] );
	return E_MYOS_SUCCESS;
}


/* virtual
myos_result_t
DP8390D::get( myos_call_t call, NVPAIR parameters[], IO::OStream& output ) const
{
   if (strcmp(call,"getAddress")==0) {
      return (myos_result_t) output.printf( "<ethernet adr=\"%X.%X.%X.%X.%X.%X\"/>",
         localAddress[0],localAddress[1],localAddress[2],
         localAddress[3],localAddress[4],localAddress[5] );
   } else if (strcmp(call,"getStatistics")==0) {
      return (myos_result_t) output.printf(
         "<ethernet rx=\"%u\" tx=\"%u\" frameErrors=\"%u\" "
         "crcErrors=\"%u\" missedPackets=\"%u\"/>",
         rxCount, txIrq, frameErrors, crcErrors, missedPackets );
   } else return IInterface::get(call,parameters,output);
}
*/


}}}} // namespace

