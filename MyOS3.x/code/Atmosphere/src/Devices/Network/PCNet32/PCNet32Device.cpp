/***************************************************************************
                          PCNet32Device.cpp  -  description
                             -------------------
    begin                : Thu Oct 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain

	This code was mostly inspired by pcnet32.c, the linux driver for this AMD card
 ***************************************************************************/
//#include "myosconfig.h"
//#if defined(CONFIG_PCNET32) || defined(COMPONENTCOMPILATION)

#include "PCNet32Driver.h"
#include "PCNet32Device.h"
#include "CPU/IOPort.h"
#include "Devices/PCI/IPCIDevice.h"
#include "Networking/Ethernet/EthernetPacket.h"

#include "debug.h"

namespace MyOS { namespace Devices { namespace Network { namespace PCNet32 {

using namespace Drivers::PCI;
using namespace Networking::Ethernet;
using Drivers::Network::PCNet32::PCNet32Driver;

// Shorthands
#define VIRTM PCNet32Driver::getInstance().iVirtualMemory

// Define this to do a dummy PCI read after write, prevent PCI posted writes
// from messing things up. Some motherboards/chipsets/processors have problems
// without this. TO DO: another solution?
#ifdef PCNET32_USE_MMIO
   #define DUMMYREAD(x) volatile u32 dummy __attribute__ ((unused)); dummy = READ32(x);
#else
   #define DUMMYREAD(x)
#endif

/// The PCNet32 chip has 32 bytes of (MM)IO space, given register indices are for bytes!
#ifdef PCNET32_USE_MMIO
   #define READ32(p)    (*((volatile u32*) (IObase+p)) & 0xffff)
   #define WRITE32(p,v)  (*((volatile u32*) (IObase+p)) = v & 0xffff)

   /// CSR88 is the only register with 32 valid bits, so dont mask (write RAP first!)
   #define READCSR88 (*((volatile u32*) (IObase+PCNET32_DWIO_RDP)))

#else
   #define READ32(p)     (inl(IObase+p)&0xffff)
   #define WRITE32(p,v)  outl(IObase+p,v&0xffff)
   #define READCSR88     inl(IObase+PCNET32_DWIO_RDP)
#endif



PCNet32Device::PCNet32Device( MyOS::Core::IComponent& c, IPCIDevice& dev )
: IEthernetDevice( c, VERSION(1,0) ), device(dev), status(UNDEFINED)
{
}

inline u16 PCNet32Device::readRAP() const
{
   return READ32(PCNET32_DWIO_RAP);
}

/// Write the Register Address Pointer with an index of a CSR / BCR register
inline void PCNet32Device::writeRAP( u16 index ) const
{
   // only 8 bits (valus from BCR/CSR_REGISTERS) are allowed, rest should be zero!
   WRITE32(PCNET32_DWIO_RAP,index);
}

inline u16 PCNet32Device::readCSR( CSR_REGISTERS index ) const
{
   /// all registers except CSR88 (ID_LOW) have only 16 valid bits
   writeRAP(index);
   return READ32(PCNET32_DWIO_RDP);
}

inline void PCNet32Device::writeCSR( CSR_REGISTERS index, u16 value ) const
{
   writeRAP(index);
   WRITE32(PCNET32_DWIO_RDP, value);
}

inline u16 PCNet32Device::readBCR( BCR_REGISTERS index ) const
{
   writeRAP(index);
   return READ32(PCNET32_DWIO_BDP);
}

inline void PCNet32Device::writeBCR( BCR_REGISTERS index, u16 value )
{
   writeRAP(index);
   WRITE32(PCNET32_DWIO_BDP, value);
}

inline void PCNet32Device::switchTo32bit()
{
   /// The chip switches to 32bit mode by doing a 32bit write to RDP. 0 is safe
   WRITE32(PCNET32_DWIO_RDP, 0);
   DUMMYREAD(PCNET32_DWIO_RDP);
}

inline void PCNet32Device::reset()
{
   /// Chip resets by reading reset register
   volatile u32 notused __attribute__((unused));
   notused = READ32(PCNET32_DWIO_RESET);

   /// The IENA bit in CSR0 is only cleared when STOP is set
   writeCSR( CONTROLLER_STATUS, STOP );

   /// After SW reset, the following registers are cleared: (p.91)
   /// CSR0, CSR3, CSR4, CSR15, CSR80, CSR100, CSR124
}

bool
PCNet32Device::initialize( IPCISupport& p ) // INITSECTION
{
#ifdef PCNET32_USE_MMIO
	#define __IO__ PCI_USE_MMIO
#else
	#define __IO__ PCI_USE_PORTIO
#endif

	// device = &dev;
	/// Must enable device to get access to registers!
	if (!p.enableDevice( device, __IO__ | PCI_MASTER | PCI_POWER_D0) ) {
		DPRINTK( "\nPCNET32: Enable device failed!" );
		return false;
	}

	const PCIDeviceInfo_t& info = device.getInfo();

#ifdef PCNET32_USE_MMIO
	IObase = (u32) info.MMIOstart;	// PCI support does mapMMIO
#else
	IObase = info.IObase;
#endif
	ASSERTION( IObase, E_ERROR );

	/// My code: Switch to 32bit mode first (needed!)
	switchTo32bit();

	// Reset the chip
	reset();

	/// Check if chip can be found: test setting CHIPID_LO index register
	u32 check = readCSR(CONTROLLER_STATUS);
	do {
		if (check==STOP) {
			writeRAP(CHIPID_LO);
			if (readRAP() == CHIPID_LO) break;
		}
		DPRINTK( "\nPCNet32 chip not found, status=%x", check);
		return false;
	} while (0);
	DPRINTK( "\nPCNet32 32bit chip found IRQ=%X", info.assignedIRQ );

	/// Read chip manufacturer code, check if valid (AMD = 0x2, lower bit always set)
	/// CSR88 is already selected above
	chipID = READCSR88;
	if ((chipID&0xFFF) != 0x003) {
		DPRINTK( "\nAMD manufacturer code not found, I got %x" ,chipID );
		return false;
	}

	/// CSR88 contains 4 bits for silicon version, CHIPID_HI too
	chipID = (chipID>>12) & 0xFFFF;  // should be 0x2624 for Am79C972 chip
	DPRINTK( "\nAMD manufacturer code found, chipID=%x", chipID );

	/// Read the local address, 3*2 bytes (stored using little endian!)
	for (u32 i=0; i<3; ++i) {
		(u16 &) localAddress[2*i] = readCSR( (CSR_REGISTERS) (PHYS_ADR0_1 + i) );
	}
	DPRINTK( "\nEthernet address %X:%X:%X:%X:%X:%X",
		localAddress[0],localAddress[1],localAddress[2],
		localAddress[3],localAddress[4],localAddress[5] );

	// Set up config block: (2^4)*4K pages (!)
	dmadata = (DMADATA*) VIRTM->allocate( 4, &dmaPhysicalAddress );
	dmadata->config.mode = SELECT_MII | DISABLE_RX | DISABLE_TX;
	dmadata->config.rxDescriptorCount = LOG2RXRINGSIZE << 4;
	dmadata->config.txDescriptorCount = LOG2TXRINGSIZE << 4;
	dmadata->config.physicalAddress = localAddress;

	// Used for accepting Multicast packets ( CRC index, bit set -> accept )
	dmadata->config.logicalAddressFilter[0] = 0;	// not used yet, better 0
	dmadata->config.logicalAddressFilter[1] = 0;	// not used yet, better 0
	dmadata->config.rxDescriptorRing = dmaPhysicalAddress + rxRingOfs();
	dmadata->config.txDescriptorRing = dmaPhysicalAddress + txRingOfs();

	initializeRings();
	DPRINTK( "\ndmaPhysical=@%x", (dmaPhysicalAddress + configOfs() ) );

   /// Program the onchip SRAM to make sure it is used ? (p.79 of Am79C972_1.pdf)
   /// => is programmed from the EEPROM already...(value 23 => 12K)

   /// Switch to 32bit access mode, both burst & non-burst access ( p.27 Am79C972_2.pdf)
   writeBCR( BCR_SOFTWARE_STYLE, 0x3 );

   /// turns on noUnderflow and burst R/W enable
   writeBCR( BCR_BURST_CONTROL,
      readBCR(BCR_BURST_CONTROL)|NoTxUnderflow|PCIBurstReadMultiple|PCI_BURSTMODE);

   writeCSR( DMATransferFIFOTreshold, (readCSR(DMATransferFIFOTreshold) & 0x0C00) | 0x0C00 );

   /// set address of initblock, initial mode: 0x3 = disable tx/rx (CSR15)
   physadr_t ibAdr = dmaPhysicalAddress + configOfs();
   writeCSR( INITBLOCK_LO, ibAdr & 0xFFFF );
   writeCSR( INITBLOCK_HI, ibAdr >> 16 );

   /// Switch to low-latency rx mode
   /// writeCSR( TESTREGISTER1, readCSR(TESTREGISTER1) | RUNT_PACKET_ACCEPT );
   /// writeBCR( SRAM_CONTROL, readBCR(SRAM_CONTROL)|LOW_LATENCY_RXMODE );

   // auto-align incoming packets to dword boundaries (not needed, rxbuffers are aligned)
   // writeCSR( ADVANCED_FEATURE_CONTROL, readCSR(ADVANCED_FEATURE_CONTROL) | RX_ALIGN_DWORD );

   /// Set up MII control register (pcnet32.c does this in open() )
   writeBCR( MII_CONTROL_STATUS, (readBCR(MII_CONTROL_STATUS) & ~0x98) | 0x20 );

   // Set auto select bit and enable led programming
   writeBCR(MISC_CONFIG, readBCR(MISC_CONFIG) | 2 | (1<<12) );

   /// Set full duplex ? NO, docs II p.20 explicitly state "Not if auto-negotiation is used!"
   // writeBCR( FULLDUPLEX_CONTROL, readBCR(FULLDUPLEX_CONTROL) | 1 );

   /// Disable TX stop on underflow, enable all ints
   writeCSR( INTMASK_DEFERAL_CTRL, GracefulUnderflowRecovery );

   /// enable Tx done interrupt inhibitor, disable magic packets
   writeCSR( EXT_CONTROL_INT1, EnableSystemInt | EnableExcessiveDef | EnableLastTxIntCheck );

   // Set some more features
   writeCSR( TEST_FEAT_CTRL, MASK_ALL | AUTOSTRIPPADDING | AUTOPADDING );

   /// Start tokenbucket timer, once per 1/2 second since chip supports max. 838 ms period
   writeBCR( SW_TIMER, (u32) (500000 / 12.8) );   /// Interrupt occurence is still disabled...

	/// Program the leds to show link_state(0), duplex(1), 100Mbps(2), rxmatch(3)
	writeBCR( LED0_STATUSCONTROL, STRETCH_PULSE | LINKSTATUS );
	writeBCR( LED1_STATUSCONTROL, STRETCH_PULSE | FULL_DUPLEX );
	writeBCR( LED2_STATUSCONTROL, STRETCH_PULSE | MBPS100 );
	writeBCR( LED3_STATUSCONTROL, STRETCH_PULSE | RXMATCH );
	writeBCR(MISC_CONFIG, readBCR(MISC_CONFIG) & ~(1<<12) );

	status = INITIALIZED;
	DPRINTK( "\nAm79C972 initialized status=%x", readCSR(CONTROLLER_STATUS) );

	return p.enableIRQ( device, *this );
}

void
PCNet32Device::initializeRings()
{
   /// Allocate a continous region of full pages (easiest for now)
   u8 *rxmem = dmadata->rxBuffers;
   physadr_t rxphys = dmaPhysicalAddress + rxBuffersOfs();

   for ( RXDESCRIPTOR *desc=dmadata->rxRing;
         desc < &dmadata->rxRing[1<<LOG2RXRINGSIZE]; ++desc) {
         desc->status = OWN | ((-PACKET_BUF_SIZE) & 0xFFFF);   // stored in 2-complement
         desc->userspace = (u32) rxmem;                  // keep the logical address here !
         desc->rxBuffer = rxphys;
         rxmem += PACKET_BUF_SIZE;
         rxphys += PACKET_BUF_SIZE;
   }

   /// clear the OWN bits in the TX ring descriptors, just in case
   memset( dmadata->txRing, 0, sizeof(dmadata->txRing) );
}

void
PCNet32Device::deinitialize()
{
	close();
	if ( dmadata ) {
		VIRTM->free( dmadata, 4 );
		dmadata = 0;
	}
	PCNet32Driver::getInstance().getPCISupport().releaseDevice( device );
}

/*************************************************
**            Interrupt processing              **
*************************************************/

void
PCNet32Device::handleError( u32 csr0 )
{
   DPRINTK( "\nPCNet32 error status=%x", csr0 );

   /// Rx ring full, some PCI chips fill up the Rx ring with error descriptors...
   if (csr0 & MISS) handleRX();
}

// virtual, IInterruptContext
void
PCNet32Device::onInterrupt( u32 message, cpu_id_t cpu ) throw()
{
#ifdef STATISTICS
   ++RxIRQCalled;
#endif

   u32 maxwork = MAXIRQWORK;

   /// Get current setting of address index register
   u32 rap = readRAP();
   register u32 csr0;

   /// Select CONTROLLER_STATUS only once (!) each iteration (might be changed in callback)
   while ( ((csr0 = readCSR(CONTROLLER_STATUS) ) & (RINT|TINT|ERR)) && --maxwork )
   {
      // PCNet32Component::getInstance().display()->cout().printf( "\nAm79C972 handleIRQ2 CSR0=%x", csr0 );
      WRITE32( PCNET32_DWIO_RDP, csr0 & ACKMASK );      // ack

      if (csr0 & RINT) handleRX();
      if (csr0 & TINT) handleTX(csr0);
      if (csr0 & ERR) handleError(csr0);
   }

   /// clear all error bits, required !
   WRITE32( PCNET32_DWIO_RDP, ACK_CLEAR );

   /// check for a timer interrupt only once (could include MII interrupts here)
   if ((csr0 = readCSR(EXT_CONTROL_INT2))&SW_TIMER_INT) {
#ifndef COMPONENTCOMPILATION
      CHECKPOINT( 'T' );
#endif
#ifdef STATISTICS
      ++timerTicks;
#endif
      ///
      ///   Here is a nasty PCI timing bug: spurious IRQ7's occur along with SW_TIMER interrupts
      ///   I've tracked it down and now believe it is due to buffered ("posted") PCI writes
      /// Perhaps they can be solved by disabling PCI write buffering somehow, but you can
      /// also force the write to memory by doing a read after the write.
      ///
      ///   It doesnt always have to be a problem, and it differs per mainboard. However, in this case
      /// it caused spurious IRQs with my board, the computer at work had problems earlier in the code
      /// It probably occurs here because there is a WRITE-after-WRITE situation
      writeCSR(EXT_CONTROL_INT2,csr0);  // reset bit
      DUMMYREAD(EXT_CONTROL_INT2);		// flush PCI write buffer
   }

   /// Restore index register
   writeRAP( rap );
}

/// Conversion from PCNet32 bits to IEthernet status bits (0,1,3 valid)
static const Networking::Ethernet::E_PACKETFLAGS matchbits[] = {
   BROADCAST_MATCH,
   MULTICAST_MATCH,
   INVALID,
   PHYSICAL_MATCH
};

static inline E_PACKETFLAGS getMatchBits( u32 status )
{
   // (status>>20) == 1,2 or 4 (broadcast, logical filter, or physical match)
   return matchbits[ ((status >> 20)&0x7)-1 ];      // index 0,1,3
}

void
PCNet32Device::handleRX()
{
   u32 entry = curRX & RXRINGMASK;
   u32 maxwork = MAXIRQWORK;
   register s32 status;
   while ( (status = dmadata->rxRing[entry].status ) >= 0 && --maxwork)   // use OWN as sign bit
   {
      RXDESCRIPTOR &desc = dmadata->rxRing[entry];

      /// check for error
      if (status & ANYERROR) {
         DPRINTK( "\nAm79C972 handleRX: errorbits=%x", status & ANYERROR );
         desc.status &= ~(ANYERROR|OWN);      // clear error status & OWN
#ifdef STATISTICS
         ++RXerrors;
#endif
      } else {
          /// subtract 4 bytes for CRC
          Packet &p = rxclient->allocateCopyCRCPacket( (u8*) desc.userspace, desc.packetSize-4 );

          // I use userspace to keep the logical address of the buffer (i.o. the DMA adr)
          rxclient->onPacket( p, getMatchBits(status) );
#ifdef STATISTICS
         ++packetsReceived;
         bytesReceived += desc.packetSize;
#endif
      }

      /// Free the buffer, proceed to next
      desc.status |= OWN;
      entry = (++curRX) & RXRINGMASK;
   }
}

void
PCNet32Device::handleTX(u32 csr0)
{
   DPRINTK( "\nPCNet32 handleTX" );

   /// only called when client wants a callback for succesful packet transmission
   for (u32 maxwork=MAXIRQWORK;maxwork>0; --maxwork, ++dirtyTX)
   {
      TXDESCRIPTOR &desc = dmadata->txRing[ dirtyTX & TXRINGMASK ];
      register s32 status = desc.status;
      if (status < 0) break;      // not TX'ed yet
      else {
      	TXDATA& data = txdata[ desc.userspace ];
      	if ((status & ERROR)==0) {
      	    rxclient->onPacketSent( * (Networking::Packet*) data.packet );
      	} else {
      	    // TODO ? packetProcessor->onPacketAborted( * data.packet );
      	}
      }
	}

   // writeCSR( TEST_FEAT_CTRL, readCSR(TEST_FEAT_CTRL) );      // ACK interrupt for tx-start
}

// virtual
myos_result_t
PCNet32Device::open( IPacketProcessor& rx, u32 mode )
{
   switch (status)
   {
	case OPENED:
		return E_MYOS_SUCCESS;	// could check that rxclient == rx
	case INITIALIZED:
	case CLOSED:
		break;
	default:
		DPRINTK( "\nWeird state value: %d", status );
		return E_MYOS_EBUSY;
   }

	rxclient = &rx;

	/// Set mode (implicitly CSR15) to default (initblock address should still be ok!)
	dmadata->config.mode = SELECT_MII /* | NO_RX_BROADCAST | PROMISCUOUS*/;       /// auto-negotiate is used

	/// enable txdone
	// writeCSR( TEST_FEAT_CTRL, readCSR(TEST_FEAT_CTRL) & ~TX_START_MASK );

	// read initblock
	writeCSR( CONTROLLER_STATUS, INIT );

	/// Wait till success
	for (int i=0; i<1000; ++i) {
		if (readCSR( CONTROLLER_STATUS ) & INITDONE ) {
        	DPRINTK( "\nPCNet32: Controller status initdone" );
			break;
		}
	}

	/// Set a new Rx polling interval, procedure on p. 128 (experiment)
	writeCSR( CONTROLLER_STATUS, STOP );
	// 2-complement, once every ms for clock@33Mhz
	writeCSR( RX_POLLING_INTERVAL, (u16) -33000 );

	/// Enable timer interrupts?
	// writeCSR( EXT_CONTROL_INT2, readCSR(EXT_CONTROL_INT2) | SW_TIMER_ENABLE /* |ENABLE_MIIINTS */ );
	writeCSR( CONTROLLER_STATUS, IENA | START );
	status = OPENED;
	DPRINTK( "\nPCNet32 after START status=%x", readCSR(CONTROLLER_STATUS) );
	return E_MYOS_SUCCESS;
}

// virtual
void
PCNet32Device::close()
{
   if (status!=CLOSED) {
      writeCSR( CONTROLLER_STATUS, STOP );   // this clears IENA
      status = CLOSED;
   }
}

// virtual
myos_result_t
PCNet32Device::sendPacket( const Networking::Packet& packet, u32 flags )
{
   if (status!=OPENED) {
      DPRINTK( "\nAm79C972 transmit: device not open" );
      return E_MYOS_ENODEV;
   }

   // Note: increment is atomic
   u32 index = (curTX++) & TXRINGMASK;
   TXDESCRIPTOR &desc = dmadata->txRing[ index ];

   /// If the TX ring is full, the OWN bit will be set
   if (desc.status & OWN) {
      DPRINTK( "\nAm79C972 sendPacket: TX queue full" );
      return E_MYOS_EBUSY;
   }

	/// correct octets for packetsize
	int octets = packet.getTxSize(); // + sizeof(EthernetPacket);

	/// Set local address
	EthernetPacket &p = packet.getDataAs<EthernetPacket>(); // - 1;
	p.setLocalAddress( localAddress );

	desc.misc 	 = CLEAR;
	desc.txBuffer = VIRTM->getPhysicalAddress(&p);

	// 2-complement packetsize
	desc.status = START_TX | ((-octets) & 0x0000FFFF) | TXDONE_INT;

	// Fill in request data too, XX only needed if onDone!=0
	TXDATA& data = txdata[ index ];
	//data.payload = buf.getData();
	//data.sender  = onDone;
	//data.act 	 = act;
	data.packet = &packet;

	/// force transmission
	// disp.cout().printf( "\nSending packet status=%x size=%d", desc.status, octets );
	writeCSR( CONTROLLER_STATUS, IENA | TDMD );

	// Could wait for completion (desc.status & OWN) == 0, but waste of cycles
	// Could check for link status: readBCR(LED0_STATUSCONTROL) & LINKSTATUS

#ifdef STATISTICS
   ++packetsSent;
   bytesSent += octets;
#endif
	return E_MYOS_SUCCESS;
}

// virtual, should be in base class (but: problems with generation of XML)
myos_result_t
PCNet32Device::getAddress( IO::OStream& out ) const {
	for (int c=0; c<6; ++c) out.printf( "%X", localAddress[c] );
	return E_MYOS_SUCCESS;
}


}}}}	// namespace

// #endif // def CONFIG_PCNET32
