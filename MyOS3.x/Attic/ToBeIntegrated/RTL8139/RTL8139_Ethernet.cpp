/***************************************************************************
                          RTL8139_Ethernet.cpp  -  description
                             -------------------
    begin                : Sun Aug 19 2001
    copyright            : (C) 2001 by jeroen
    email                : jeroen@thebem
 ***************************************************************************/
#include "new.h"  // placement

#include "RTL8139_Ethernet.h"
#include "RTL8139_chip.h"

#include "Drivers/Network/RTL8139/RTL8139_CIMPL.h"

using namespace ScreenStuff;

namespace RTL8139 {

static const TXCONFIG DEFAULT_TX_CONFIG = {
            false,      /// clearAbort command
            RESERVED_BITS,
            0,          /// TxRetry count, 16 + value
			0x6,        /// MaxTxDMABurst, 8139too:7, Becker:4		Realtek:6
			RESERVED_BITS,
			false,      /// 'false' means append CRC
			TXCONFIG::NORMAL,
			RESERVED_BITS,
			0,          /// indicates presence of RTL8139-A rev. G chip
			TXCONFIG::IFG_960ns_default,	// unsure about numbering, value 3 could be 840ns !
			TXCONFIG::NONE,
			RESERVED_BITS
};	// 0x77000600

static const RXCONFIG DEFAULT_RX_CONFIG = { 	
            0, 1, 1, 1, 0, 0, RXCONFIG::EEPROM9346,
			true,			// NoRingWrap, needed to prevent serious problems!
			0x6, 			// maxRxDMABurst	8139too:7 Becker:4 Realtek suggested:6
			RXCONFIG::_32K_16,
			0x4, 	// RxFIFOTreshold => 8139too : 7 (whole packet), Becker : 4
			false,  /// No small error packets
			false,  /// allow earlyRxIRQ for packet of any protocol
			RESERVED_BITS,
			0,      // earlyRxTresholdfactor, "should not be 0" if earlyRxsize is set...
			RESERVED_BITS
};	// 0x0000948E


// put in Rx buffer, at start of received packet
struct PACKETHEADER		
{
	bool ReceiveOK : 1;		        // I assume this is mutually exclusive with the following ??
	bool FrameAlignError : 1;
	bool CRCError : 1;
	bool LargerThan4K : 1;		    // could this occur, with a max ethernet framesize of 1536 ?
	bool SmallerThan64Bytes : 1;	// mediaheader+data+CRC < 64
	bool InvalidSymbol : 1;		    // 100BaseTX only	
//	u8 __reserved__ : 7;
	enum { BROADCAST=0x080, PHYSICAL=0x100, MULTICAST=0x200 } match : 10;

	u16 size;			// size of packet, including 4 bytes for CRC (!)
	u32 startOfPacket;

	inline EthernetPacket* getPacket() const    { return (EthernetPacket*) &startOfPacket; }
	inline u32 bits() const                			{ return *((u32*)this); }

    /// I_Ethernet flags
    inline u32 flags() const
    {
        return (match >> 8)+1;  // 1, 2 or 3
    }
};

enum PACKETSTATUS
{
	RX_OK			    		= 0x01,
	FRAME_ALIGN_ERROR	= 0x02,
	CRC_ERROR		    	= 0x04,
	LARGER_THAN_4K		= 0x08,
	SMALLER_THAN_64B	= 0x10,
	INVALID_SYMBOL		= 0x20,

	SOMETHING_WRONG		=   FRAME_ALIGN_ERROR | CRC_ERROR | LARGER_THAN_4K |
	                        SMALLER_THAN_64B | INVALID_SYMBOL
};


/*
	General considerations
	
	*	Must be *VERY* efficient, as little memory copying as possible
	*	Think about generating packets in a controlled (timed) manner (ie at regular intervals, perhaps using chip's timer)
	*	Chip has 2 FIFO buffers of 2K, one for sending and one for receiving
	*	

The SEND algorithm
	
	I can see 2 options:
	1) 	Allocate 4 fixed Tx buffers, application(s) do RequestBuffer() which blocks when no buffers are available
	2)	Use application-provided physical buffers in SendPacket( buffer* ), reprogram the RTL8139 Tx ringbuffer every time
		also block when all ringbuffers are busy (this is what linux seems to do, with optional copying for alignment )

	Option 2 seems more attractive, because then it is possible to build packets on the stack and send them eg
	void sendFunction()
	{
		EthernetPacket p;		// must be aligned !
	
		p.dest = x;
		{ build a packet in p.data }
		EthernetDevice.send( p );	// will block when no buffers are available, so stack is destroyed after succesful transmit
	}
	
	If more efficient, caller can also opt to pre-define an array of Ethernet structures (not on stack) and build without copying data
	
	Hmmm. I just think of a problem: physical addresses should be in subsequent physical pages, suitable for DMA !!! This could be achieved
	in the above example, but would require that
		1)	Either packetsize < 4096 OR the data segment is guaranteed to be allocated sequentially (2nd is too much to ask I think)
		2)	Some kind of WBINVD on p to make sure the data is actually in memory before transmitting

	>>	8139too does the following: When the last of the 4 descriptors is used, the incoming netif_queue is stopped
	>>	The queue is started again when the interrupt procedure has succesfully acknowledged a transmit
	
		
	My guess is that the data provided to the chip should start with a valid ethernet header. I also guess that linux' pointers in sk_buff
	structure point into the data buffer
	
	
The RECV algorithm
	
	The chip has a single Rx buffer (ring) that wraps. For high performance, it would be nice if packets
	could be processed in-memory rather than copying them out of the way.
	
	Perhaps it is possible to switch between multiple Rx rings dynamically (ie reprogram the chip)?	
	I tried this:

	- CurAddress cannot be changed, it is read-only and changing causes lock-up
	- startOfRxBuffer *can* be changed to ensure that the chip writes to the correct 4K page, BUT...
	  when a Rx IRQ occurs, the chip does *not* stop writing to memory! CurAddress changes even during the
	  RX IRQ handler, which means that changing the startOfRxBuffer causes major packet loss!
		
	So....
	1) there must be a fixed area of memory allocated for the Rx part, large enough to hold many packets
	2) Can still try to process some packets (small, eg ACK) in-memory, but sometimes copying is unavoidable...	
*/

#define SYNCHRONOUSPACKET ((EthernetPacket*) -1)

C_RTL8139_Ethernet::C_RTL8139_Ethernet( C_RTL8139_PCIDevice &dev )
  : device(dev),
		client(0), curRx(0)
  , bytesPerSecondLimit( NO_LIMIT )
  , bytesAllowedThisSecond( NO_LIMIT )
  , packetsQueued(0)
  , status( S_INITIALIZING )
{
    /// @see initialize
}

I_PCIDevice&	// virtual
C_RTL8139_Ethernet::getDevice() const
{
	return device;
}

bool
C_RTL8139_Ethernet::initialize( LINEARADDRESS MMIOstart )
{
	this->chip = (MMIO*) MMIOstart;

	/// read the local address from the chip
	u32 adrdwords[2];
	adrdwords[0] = chip->IDR[0];
	adrdwords[1] = chip->IDR[1];    // 2 bytes unused
	memcpy( (void*) &localAddress, &adrdwords, sizeof(ETHERNETADDRESS) );

	// allocate 32K+16 bytes rxBuffer here: chip does not wrap when using 64K
	I_KernelMemory *mem = GETCOMPONENT( RTL8139C ).kernelmem();
	rxBuffer = (u8*) mem->allocate( GETCOMPONENT( RTL8139C ),
		DEFAULT_RX_CONFIG.buffersize() + 0x1000 - 16,    // for now, whole pages
		KM_DMA    // DMA pages -> dont cache them!
	);
	if (!rxBuffer) {
		PRINTM( "\nFailed to allocate RxBuffer" );
		return false;
	}

  /// Program the physical address of the rxBuffer
	chip->RxPhysBufferStart = mem->getMapping( rxBuffer );

  // unlock Config[01234] and BMCR register writes
  chip->Config9346 = CONFIG_WRITE_ENABLE;
                    	
  // program config3
  CONFIG3 cfg3 = chip->config3;
  cfg3.enableMagicPacketWakeup = false;
  chip->config3 = cfg3;

  // program config4
	chip->config4 = (chip->config4 | AutoClearFIFOOverflow) & ~LANWakePattern;

	// enable auto-negotiation (in 2 separate steps, generates linkchange, takes some time)
	chip->BasicModeControl |= autoNegotiate;
	chip->BasicModeControl |= resetAutoNegotiate;

	/// Indicate the driver has finished loading		
	chip->config1 |= (LED0 | DRIVER_LOADED);

	// lock Config[01234] again
	chip->Config9346 = NORMAL_MODE;

	status = S_INITIALIZED;
	GETCOMPONENT( RTL8139C ).display()->printf( "\nRTL8139C chip initialized" );
	return true;
}

bool
C_RTL8139_Ethernet::takeOver( C_RTL8139_Ethernet &from )
{
	/// Re-open device if it was open when the old one was closed
	if (from.status==S_DEINITIALIZED_WAS_OPEN) resume();	
	if (from.client) {
		client = from.client;
		clienthandle = from.clienthandle;
		*clienthandle = &from;
		bytesPerSecondLimit	= from.bytesPerSecondLimit;
		clientHeartbeatFrequency = from.clientHeartbeatFrequency;
		return true;
	}
	return false;
}

void
C_RTL8139_Ethernet::deinitialize()
{
	if (chip) {
		bool wasopen = (status==S_OPENED);
		close();
		status = wasopen ? S_DEINITIALIZED_WAS_OPEN : S_DEINITIALIZED_WAS_CLOSED;

		/// get the memory interface & free the memory
		if (rxBuffer) {
			I_KernelMemory *mem = GETCOMPONENT( RTL8139C ).kernelmem();
			mem->free( rxBuffer, DEFAULT_RX_CONFIG.buffersize() + 0x1000 - 16, KM_DMA );
			rxBuffer=0;
		}

		if (clienthandle) {
			client->onDeviceRemoved( *this );
	   	*clienthandle = 0;		
		}
	}
}


void
C_RTL8139_Ethernet::close()
{
	if (status!=S_CLOSED) {
		status = S_CLOSED;

		/// Gracefully wait for pending Tx
		for (u32 desc=0; desc<3; ++desc) {
			u32 retry = MAX_RETRY_COUNT_WHILE_BLOCKING;
			while ((!chip->TxStatus[desc].TxDMAComplete) && --retry )
			{
	
			};
		}

		/// Stop the chip
		chip->IMR = DISABLE_ALL;
		chipreset();
	}
}
	
bool
C_RTL8139_Ethernet::setClient(
	I_EthernetDevice::Client &c, I_EthernetDevice **handle, u32 heartbeat, RATELIMIT sendLimitRateBPS )
{
	if (client==0) {
		client = &c;
		clienthandle = handle;
		clientHeartbeatFrequency = heartbeat;
		if (sendLimitRateBPS) bytesPerSecondLimit = sendLimitRateBPS;
		return true;
	}
	return false;
}

void
C_RTL8139_Ethernet::removeClient( Client &c )
{
	if (client==&c) {
		client=0;
    bytesPerSecondLimit = NO_LIMIT;
		clienthandle = 0;
	}
}

ETHERNETADDRESS&
C_RTL8139_Ethernet::getLocalAddress() const
{
	return  localAddress;
}

void
C_RTL8139_Ethernet::chipreset()
{
	// Soft reset the chip.
	chip->Command = ResetChip;

	// Check that the chip has finished the reset.
	for (int i = 1000; i > 0; i--) {				
		// NOTE: Linux includes a 'barrier' here		
		if ( (chip->Command & ResetChip) == 0) break;
		GETCOMPONENT( RTL8139C ).timing()->delay( 10, Timing::us );
	}	
}


bool
C_RTL8139_Ethernet::resume()
{
	switch (status)
	{
	case S_OPENED:
		return true;
	case S_DEINITIALIZED_WAS_OPEN:
	case S_DEINITIALIZED_WAS_CLOSED:
	case S_INITIALIZING:
	case S_UNDEFINED:
		return false;
	default:
		chipreset();
	}

	// 8139too locks Config1234 here
	
	// Must enable Tx/Rx before setting transfer thresholds!
	curRx = 0;
	chip->Command = RxTxEnable;
	chip->RxConfig = DEFAULT_RX_CONFIG;		// default settings
	chip->TxConfig = DEFAULT_TX_CONFIG;

	// Clear missed count, start Rx process
	chip->MissedPacketCounter = 0;

	/// Tried this, but doesnt seem to work. earlyRxstatus = 0xA all the time
	// added: set the earlyRx level to UDP headersize ex. checksum (14+20+6)
	// chip->earlyRxsize = 40;  // docs: must be multiple of 4
	chip->earlyRxsize &= 0xF000;            /// disable earlyRx

	// let the timer fire once per second for traffic shaping
	chip->TimerMark = TIMER_TICKS_PER_SECOND;	

	// Enable all known interrupts by setting the interrupt mask. (could disable TxOK...)
	chip->IMR = ALL_INTR;

	status = S_OPENED;	
	PRINTM( "\nRTL8139 chip resumes functioning..." );
	return true;
}

I_EthernetDevice::RESULT
C_RTL8139_Ethernet::transmit( EthernetPacket &p, size_t payload, u32 flags )
{
	CHECKPOINT( 'T' );

	// chip can only send 4-byte aligned memory
	ASSERTIONv( (((u32)&p) & 0x3) == 0, Debug::AS_FATAL, &p );	

	if (status!=S_OPENED) return DEVICE_NOT_OPEN;
	else if (!linkUp) return LINK_DOWN;

	// include ethernet headersize
	payload += EthernetPacket::HEADERSIZE;

	// packet must be 4-byte aligned, min. size is 60 bytes (+4 CRC)
	u8 desc = packetsQueued.ReadAdd(1) & 0x3;
		
	// blocking wait here, no semaphore: this driver only provides the mechanism, not the policy
	u32 retry = (flags&E_BLOCKING_TXQUEUE) ? MAX_RETRY_COUNT_WHILE_BLOCKING : 1;
	while ((!chip->TxStatus[desc].TxDMAComplete) && --retry )
	{
        // do something useful here ? allocate memory perhaps ? could read timer to see how long there is!		
	};
	if (retry==0) return BLOCKING_TXQUEUE_TIMEOUT;
		
	// separate loop, to not continuously read the PCI register
	retry = (flags&E_BLOCKING_TXLIMIT) ? MAX_RETRY_COUNT_WHILE_BLOCKING : 1;
	while ((( (s32)payload > bytesAllowedThisSecond) || (packetcache[desc].packet != 0))
			&& --retry)
	{
        // do something !			
	}		
	if (retry==0) return BLOCKING_TXLIMIT_TIMEOUT;

	// mark the cache
	packetcache[desc].packet = (flags&E_BLOCKING_TX) ? SYNCHRONOUSPACKET : &p;
	packetcache[desc].octets = payload;
	packetcache[desc].notify = (flags&E_NOTIFY_TX);

	/// Fill in the local address
	p.setLocalAddress( localAddress );

	// get the physical address
	u32 txAddress = GETCOMPONENT( RTL8139C ).kernelmem()->getMapping(&p);
	chip->TransmitStartAddress[ desc ] = txAddress;	

  // Do padding, currently *not* zeroed...
  if (ENFORCE_MIN_PACKETSIZE && (payload<EthernetPacket::MINFRAMESIZE)) {
		payload=EthernetPacket::MINFRAMESIZE;
	}
	// PRINTM( "\nSending ethernet packet bytes=" << payload );
  chip->TxStatus[desc].startTx( payload );

  // wait for DMA completion (but dont keep polling PCI memory...)
  if ((flags&E_BLOCKING_TX) && Processor::InterruptsEnabled()) {
	  retry = MAX_RETRY_COUNT_WHILE_BLOCKING;
		while ((packetcache[desc].packet) && --retry)
		{
			// Note: this only works with interrupts *enabled* !!
		};
		if (retry==0) return BLOCKING_TX_TIMEOUT;
	}

	/// Always count, even if there may be errors...
#ifdef STATISTICS
	++packetsSent;
	bytesSent += payload;
#endif
  bytesAllowedThisSecond -= payload;
	return SUCCESS;
}

void
C_RTL8139_Ethernet::handleTX( u32 status )
{
	// read once (!)
	u32 allTx = (u32) chip->TransmitStatusAll;

	// Screen::POSITION debug( 23,0 ); debug << SetBase(16);
	// debug << "TxIRQ, status = " << status << "allTX=" << allTx << ",bps left=" << bytesAllowedThisSecond;

	// just clear abort once for all Tx descriptors, should be enough
	if (allTx & TXABORT_bits) {
		chip->TxConfig.clearAbort( DEFAULT_TX_CONFIG );
	}

    // normal state transitions	for a packet transmission:
    //  TDMAComplete = 0
    //      possible errors:                                      Result:
    //              Tx FIFO underrun                              TxUn=1, but packet transmitted
    //              Excessive collisions (>TxErr setting)         TAbort = 1
    //              OutofWindow collision, CarrierSenseLost       status.TxErr = 1, probably TOK=0
    //  > TDMAComplete = 1 -> TOK = 1

    // I assume that, once TDMAComplete=1, I can transmit the next packet in transmit()

	if (client==0) return;
    	
	// go by all descriptors
	for (u32 desc=0; allTx && (desc<4); ++desc, allTx >>= 1)
	{
		if (allTx&0x0001) {			// check if TDMAComplete

			// invalidate cache to allow next Tx
			EthernetPacket *p = packetcache[desc].packet.Replace(0);

			// synchronous packets must be skipped here
			if (p && p!=SYNCHRONOUSPACKET && packetcache[desc].notify)
			{
				if (allTx&0x1100)   // TOK | FIFOUnderrun (could adjust some settings then)
        {
					client->onPacketSent( *p, bytesAllowedThisSecond );
				} else if (allTx&0x0010) {    // TxAborted, could return packet for retry
					client->onPacketAborted( *p );
        } else BUG( "No TXStatus bits set?" );			
			}
			// else not uncommon: all bits remain set until next Tx, and synchronous
		}
		allTx &= 0x0000EEEE;			// clear low bit of each nibble
	}
}

void
C_RTL8139_Ethernet::handleError( u32 status, int linkChanged )
{
	if (linkChanged) {
		linkUp = (chip->mediastatus & LinkFail)==0;
		GETCOMPONENT(RTL8139C).display()->printf( "\nRTL8139C linkchange linkUp=%b", linkUp );
	} else {
		GETCOMPONENT(RTL8139C).display()->printf( "\nRTL8139C handleError status=%x", status );
	}
}

void
C_RTL8139_Ethernet::handleIRQ()		// non-static interrupt handler
{
	unsigned link_changed=0, count=MAX_WORK_PER_IRQ;
	do {
		// acknowledge interrupt ASAP, should use 32-bit calculations !
		register IRQSTATUS status = chip->ISR;
		if (status==0xFFFF) break;			        // bogus value				

		if (status & RxUnderrun_LinkChange) {
			link_changed = chip->ChipStatusConfig & CSCR_LinkChangeBit;
		}

        // dont ACK the Rx yet?? Becker does, 8139too not		
		chip->ISR = (IRQSTATUS) (status);	

		// at least one known cause
		if (status & ALL_INTR) {			
			
			/// enable interrupts unconditionally (flags on stack, they were on)
			Processor::AllowInterrupts( true );		
		
			if (status & RxAck) {   // note : also when errors occur
				handleRX( status );
			}

			// NOTE:
			if (status & (TxOK|TxErr)) {
				handleTX( status );
			}
	
			if (status & TimerInterrupt) {
				bytesAllowedThisSecond = bytesPerSecondLimit;
				chip->Timer = 0;		// restart counter
				CHECKPOINT('T');
			}
	
			// single check for uncommon events/errors
			if (status & (SystemError|RxUnderrun_LinkChange|RxBufferOverflow|RxFIFOOverflow|RxErr)) {
				handleError( status, link_changed );
			}
		} else {
			break;
		}
	} while( --count );
	
	// too much work limit reached
	if (count==0) {
		REMARK( "Too much work limit reached" );
		chip->ISR = CLEAR_INTR;	
	}	
}

void
C_RTL8139_Ethernet::handleRX( u32 status )
{
#ifdef STATISTICS
	++RxIRQCalled;
#endif

	/// Prevent recursive Rx interrupts	
	static int recursion=0;
	if (++recursion > 1) {
		--recursion;
#ifdef STATISTICS
		++RXrecursion;
#endif
		CHECKPOINT('R'); return;
	}
	
	while ((chip->Command & RxBufferEmpty) == 0)
	{
		// buffersize == 32K (0x8000) + 16 bytes
		PACKETHEADER *packetinfo = (PACKETHEADER*) &rxBuffer[curRx & 0x7FFF];

		if ( packetinfo->bits() & SOMETHING_WRONG ) {
#ifdef STATISTICS
			++RXerrors;
#endif
			CHECKPOINT( 'e' );
			
			// reset receiver
			curRx = 0;
			chip->Command = TxEnable;
			chip->RxConfig = DEFAULT_RX_CONFIG;
			chip->Command = RxTxEnable;

			// 8139too does not do this, but I think its safe to abort here			
			break;
		} else {
		    size_t size = packetinfo->size - 4;     // exclude CRC

				/// Sanity check, fails if we get lost in the ring
        if (size <= EthernetPacket::MAXFRAMESIZE) {

             /* Added: earlyRx
             Screen::cursor() << "\nearlyRx=" << (u8) chip->EarlyReceiveStatus;
             if (chip->EarlyReceiveStatus.EarlyRxOK) {
            		client->receiveEarly( *packetinfo->getPacket(), size, packetinfo->flags() );
          	 } else */
					if (client) client->receive(
						*packetinfo->getPacket(),
						size-EthernetPacket::HEADERSIZE,
						packetinfo->flags()
					);

#ifdef STATISTICS
          bytesReceived += size;
          ++packetsReceived;
#endif

          // rounded up: size (incl CRC) + packetinfo header(4)
          curRx = (curRx + (size+4) + 4 + 3) & ~3;

          // - safety margin(0x10), see example code RealTek
          chip->RxPacketRead = curRx - 0x10;

          // !NEEDED! wait until write succeeds, not '!=' but '<' prevents hanging
          while (chip->RxPacketRead < (curRx-0x10));
				} else {
        	CHECKPOINT( '@' );
          /// What to do ? try to reset pointer ?
				}
		}
	}
	--recursion;
}

#ifdef STATISTICS
s32
C_RTL8139_Ethernet::statistics( PacketWriter &buf ) const
{
	buf.printf( "\n<STATS"
		" RX_Packets=\"%d\" RX_Octets=\"%d\" RX_Recursion=\"%d\""
		" TX_Packets=\"%d\" TX_Octets=\"%d\"/>",
		packetsReceived,bytesReceived,RXrecursion,packetsSent,bytesSent
	);
	return buf.getBytesWritten();
}

#endif	// STATISTICS

};  // namespace
