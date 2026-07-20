/***************************************************************************
                          C_3c90xcEthernet.cpp  -  description
                             -------------------
    begin                : Sat Oct 20 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include <new>
#include "3c90xcCIMPL.h"
#include "3c90xcEthernet.h"
#include "IOPort.h"
#include "Core/PCIDevice.h"
#include "EthernetPacket.h"

// debug
#ifndef COMPONENTCOMPILATION
#include "Screen.h"
#include "Trace.h"
#endif

namespace _3c90xc {

#ifdef _3c90xc_USE_MMIO
	#define READ32(p) 		(*((volatile u32*) 	(IObase+p)))
	#define WRITE32(p,v)  (*((volatile u32*) 	(IObase+p)) = v )
	#define READ16(p) 		(*((volatile u16*) 	(IObase+p)))
	#define WRITE16(p,v)  (*((volatile u16*) 	(IObase+p)) = v )
	#define READ8(p) 			(*((volatile u8*) 	(IObase+p)))
	#define WRITE8(p,v)  	(*((volatile u8*) 	(IObase+p)) = v )
#else
	#define READ32(p) 		inl(IObase+p)
	#define WRITE32(p,v)  outl(IObase+p,v)
	#define READ16(p) 		inw(IObase+p)
	#define WRITE16(p,v)  outw(IObase+p,v)
	#define READ8(p) 			inb(IObase+p)
	#define WRITE8(p,v)  	outb(IObase+p,v)
#endif

C_3c90xcEthernet::C_3c90xcEthernet( C_3c90xcDevice &dev )
	: device(dev), maxBytesPerSecond(NO_LIMIT), bytesLeftThisSecond(NO_LIMIT), status(UNDEFINED)
{

}

I_PCIDevice&	// virtual
C_3c90xcEthernet::getDevice() const
{
	return device;
}

bool
C_3c90xcEthernet::setClient( Client& c, I_EthernetDevice **handle,
		u32 heartBeatPeriodInSeconds, RATELIMIT sendRateLimitBytesPerSecond /*=0*/ )
{
	if (client==0) {
		client = &c;
		if (sendRateLimitBytesPerSecond) maxBytesPerSecond = sendRateLimitBytesPerSecond;
		clienthandle = handle;
		clientHeartbeatFrequency = heartBeatPeriodInSeconds;
		return true;
	} else return false;
}

void
C_3c90xcEthernet::removeClient( Client &c )
{
	if (client==&c) {
		maxBytesPerSecond = NO_LIMIT;
		client = 0;	
	}
}

#ifdef STATISTICS
s32
C_3c90xcEthernet::statistics( PacketWriter &buf ) const
{
	return -1;
}
#endif

inline void C_3c90xcEthernet::selectWindow( u8 win )
{
	/// win should be 0..7
	WRITE8( IntStatusCmd, SelectRegWindow + win );
}

void		// Delay for the specified amount of time, by causing an interrupt
C_3c90xcEthernet::delay( u32 ns )
{
	// assuming countdownMode 0, counterSpeed set to get 320 ns ticks (4 byte times)
	timerFired = false;
	WRITE16(CountDown, ns / 320);
	while (!timerFired);
}

void 		// read once and store
C_3c90xcEthernet::readEEPROM()
{
	selectWindow(0);
	for (u32 i = 0; i < sizeof(eepromData)/sizeof(u16); i++) {
		WRITE16( WIN0_EEPROMCommand, 0x80 + i );		// 0x80 == EEPROM read base address
		// Pause for at least 162 us. for the read to take place
		for (s32 timer = 10; timer >= 0; timer--) {
			// delay(162000);
			GETCOMPONENT(C_3c90xc).timing()->delay( 162, Timing::us );
			if ((READ16(WIN0_EEPROMCommand) & 0x8000) == 0) break;
		}
		eepromData[i] = READ16(WIN0_EEPROMData);
	}
}


bool
C_3c90xcEthernet::initialize( const PCIDEVICEINFO &info ) /* INITSECTION */
{
#ifdef _3c90xc_USE_MMIO
	IObase = (u32) info.MMIOstart;
#else
	IObase = info.IObase;
#endif
	I_Display &disp = *GETCOMPONENT(C_3c90xc).display();
	if (IObase==0) {
		disp.printf( "\nIO not properly mapped for 3c90xc" );
		return false;
	}

	/// Issue general reset command
	globalReset();

	/// Reset physical layer
	

	// Set up config block
	void *dmaMem = GETCOMPONENT( C_3c90xc ).allocate( sizeof(DMADATA), KM_DMA );
	if (!dmaMem) return false;
	dmaPhysicalAddress = GETCOMPONENT( C_3c90xc ).kernelmem()->getMapping(dmaMem);
	dmadata = new (dmaMem) DMADATA();
	initializeRings();

	/// Program UpListPtr & DnListPtr (can only be written once, must be 8-byte aligned)
	WRITE32( UpListPtr, dmaPhysicalAddress + OFFSET_OF(DMADATA,rxRing) );
	WRITE32( DnListPtr, dmaPhysicalAddress + OFFSET_OF(DMADATA,txRing) );

	/// Set timer assumptions: mode 0, high speed
	WRITE32( DMAControl, (READ32(DMAControl) | counterSpeed) & ~countdownMode );

	/// TO DO: enable timer interrupt, modify handleIRQ()

	// Set some options
	selectWindow(3);	
	WRITE16( WIN3_MacControl, READ16(WIN3_MacControl) | enableFullDuplex );

	/// read the EEPROM for physical address
	readEEPROM();

	/// See p.71 for EEPROM layout, this is the OEM node Address (manual: need byte swapping!)
	(u16&) localAddress[0] = htons16( eepromData[0xa] );
	(u16&) localAddress[2] = htons16( eepromData[0xb] );
	(u16&) localAddress[4] = htons16( eepromData[0xc] );

	disp.printf( "\nEEPROM read, physical address=%X:%X:%X:%X:%X:%X",
		localAddress[0], localAddress[1], localAddress[2],
		localAddress[3], localAddress[4], localAddress[5]
	);

	/// Poke the address into the window2 registers (manual: 3x word, linux: 6x byte )
	selectWindow(2);
	WRITE16( WIN2_PhysAdr01, (u16&) localAddress[0] );
	WRITE16( WIN2_PhysAdr23, (u16&) localAddress[2] );
	WRITE16( WIN2_PhysAdr45, (u16&) localAddress[4] );

	status = INITIALIZED;
	return true;
}

void
C_3c90xcEthernet::initializeRings()
{
	/// Allocate a continous region of full pages (easiest for now)
	PHYSICALADDRESS rxphys = dmaPhysicalAddress + OFFSET_OF(DMADATA, rxRing);

	/// Must be aligned on 8-byte boundaries
	ASSERTION( (rxphys & 0x7) == 0, Debug::AS_FATAL );

	/// Do all descriptors, but overwrite the last one to wrap the ring
	RXDESCRIPTOR *desc;
	for ( u32 i=0; i<=RXRINGMASK; ++i) {
		desc = &dmadata->rxRing[i];
		desc->nextBuffer = (rxphys+=sizeof(RXDESCRIPTOR));
		desc->status = impliedBuffer;					// implied buffer of 1528 bytes directly after desc.
	}
	/// Wrap the ring
	desc->nextBuffer = dmaPhysicalAddress + OFFSET_OF(DMADATA, rxRing);

	/*
		p. 100 : The chip uses DnPolling iff
			a) a NULL ptr is read as DnNextPtr in a TXDESCRIPTOR
			b) The packet pointer to by DnNextPtr contains a scheduling time in the future
		p. 99: The value in DnListPtr is preserved when DnPolling is used. This means DnListPtr
			can only be written once (writes are only accepted if it is 0) when using polling
	
		p. 99: If using packet scheduling, polling is required

		This means the TX descriptors can not be chained in a ring
	*/
	memset( dmadata->txRing, 0, sizeof(dmadata->txRing) );
}


void
C_3c90xcEthernet::deinitialize()
{
	close();
	if (dmadata) {
		GETCOMPONENT( C_3c90xc ).free( dmadata, sizeof(DMADATA), KM_DMA );	
		dmadata = 0;
	}
}

bool
C_3c90xcEthernet::open( /* params? */ )
{
	switch (status)
	{
	case OPENED:
		return true;
	case INITIALIZED:
	case CLOSED:
		break;
	default:	
		return false;
	}

	/// Set RxFilter
	WRITE16(IntStatusCmd, SetRxFilter | physicalMatch | broadcastMatch );

	// Set appropriate interrupts: bits that can become set, and bits that generate IRQ
	WRITE16(IntStatusCmd, SetIndicationEn | HostError | RxComplete | TxComplete | LinkChange );
	WRITE16(IntStatusCmd, SetIntrEnable | HostError | RxComplete | TxComplete | LinkChange );
	// could add timer ?

	/// Enable TX&RX
	WRITE16(IntStatusCmd, TxEnable);
	WRITE16(IntStatusCmd, RxEnable);

	/// start polling (value is 4 byte times at 100Mbps)
	WRITE8( DnPoll, 100 );
	WRITE8( UpPoll, 100 );

	status = OPENED;
	return true;	
}


void
C_3c90xcEthernet::close()
{
	if (status!=CLOSED) {
		status = CLOSED;
		WRITE16(IntStatusCmd, RxDisable);
		WRITE16(IntStatusCmd, TxDisable);		
	}
}

void C_3c90xcEthernet::globalReset()
{
	WRITE16(IntStatusCmd, TotalReset);
	while ( READ16(IntStatusCmd) & CmdInProgress );
}

void C_3c90xcEthernet::TXReset()
{
	WRITE16(IntStatusCmd, TxReset);
	while ( READ16(IntStatusCmd) & CmdInProgress );
	WRITE8( DnPoll, 100 );
}

inline void C_3c90xcEthernet::RXReset( u32 resetbits )
{
	WRITE16(IntStatusCmd, RxReset | resetbits );
	while ( READ16(IntStatusCmd) & CmdInProgress );

	if (!(resetbits&dontResetReceiver)) {
		/// Set RxFilter again (has been cleared)
		WRITE16(IntStatusCmd, SetRxFilter | physicalMatch | broadcastMatch );
		WRITE8( UpPoll, 100 );
	}

}


void
C_3c90xcEthernet::handleError( u32 status )
{
	GETCOMPONENT(C_3c90xc).display()->printf( "\nhandleError status=%x", status );
	if (status & LinkChange) {
  	// p. 119: read AutoNegExpansion register to acknowledge (you can disable this interrupt...)
		// .. but p. 124: reading IntStatusAuto also clears this interrupt		
	} else {
		// p.122: Hosterror requires global reset
		globalReset();
		WRITE8( UpPoll, 100 );		
		WRITE8( DnPoll, 100 );
	}
}

void
C_3c90xcEthernet::handleIRQ()
{
#ifndef COMPONENTCOMPILATION
	CHECKPOINT( '~' );
#endif

	u32 maxwork = MAXIRQWORK;

	// reading this i.o. IntStatusCmd is better, p.22
	// from p.124 : this acknowledges active interrupts, and clears the interrupt enable register
	u32 status = READ16( IntStatusAuto );		

	/// Check the most common case, this rules out all 1's and host errors
	if ((status & (IntLatch|HostError))==IntLatch) {
		while ( status & (IntLatch|RxComplete|RxEarly|TxComplete) && --maxwork)
		{
			if (status&(RxComplete|RxEarly)) handleRX(status);
			if (status&TxComplete) handleTX();
			status = READ16( IntStatusAuto );
		}
	}
	if (status&HostError) handleError(status);

}

void
C_3c90xcEthernet::handleRX( u32 status )
{
	GETCOMPONENT(C_3c90xc).display()->printf( "\nhandleRX status=%x", status );

	/// Find out which packet(s)
	u32 offset = READ32(UpListPtr) - dmaPhysicalAddress;
	RXDESCRIPTOR *desc = (RXDESCRIPTOR*) (((u32)dmadata) + offset);
	while (desc->status & upComplete)
	{
		EthernetPacket *packet = (EthernetPacket*) desc->buffer;
  	if ((desc->status & upError)==0)
		{
			client->receive( *packet,
				(desc->status & packetLengthMask) - 4 - EthernetPacket::HEADERSIZE, bytesLeftThisSecond );
		} else {
			GETCOMPONENT(C_3c90xc).display()->printf( "\nhandleRX error packetstatus=%x", desc->status );

			// TO DO: set some options
			RXReset(0);
		}

		/// Clear the status bits (especially upComplete), go to next descriptor and wrap if needed
		desc->status = impliedBuffer;		
		if (++desc > &dmadata->rxRing[RXRINGMASK]) desc = dmadata->rxRing;
	}

}

void
C_3c90xcEthernet::handleTX()
{
	/// check for TX errors: p.103 says there is a queue of 32 status entries, get next by writing
	u8 status = READ8( TxStatus );
	if (status & TX_TxComplete) {

		// advance status queue (ACK)
		WRITE8( TxStatus, 0 );

		#ifndef COMPONENTCOMPILATION
		Screen::cursor() << "\nC390xc Tx error, status=" << status;
		#endif

		/// These conditions require TxReset (can do selective reset, see p. 138)
		if (status & (TxUnderrun|TxJabber)) {
			TXReset();
			/// @note: TXStartTresh, TXReclaimRefresh, and DNListPtr are all destroyed !
		}

		/// These conditions require TxEnable (I added TxJabber, guess after a reset Tx is disabled...
		if (status & (TxStatusOverflow|maxCollisions|TxUnderrun|TxJabber)) {
			WRITE16( IntStatusCmd, TxEnable );
		}
	}
}

I_EthernetDevice::RESULT
C_3c90xcEthernet::transmit( EthernetPacket &p, size_t octets, u32 flags )
{
	if (status!=OPENED) return DEVICE_NOT_OPEN;
	size_t retries = (flags&E_BLOCKING_TXQUEUE) ? MAX_RETRY_COUNT_WHILE_BLOCKING : 1;

	u32 entryIndex = curTX++ & TXRINGMASK;
	TXDESCRIPTOR &entry = dmadata->txRing[ entryIndex ];

	/// Alternatively, construct a chain of descriptors here
	while ( !(entry.frameStartHeader & DOWNLOAD_COMPLETE) && (--retries)) {
		// do something useful ?
	}
	if (retries==0) return BLOCKING_TXQUEUE_TIMEOUT;

	/// correct octets for packetsize
	octets += sizeof(EthernetPacket);

	/// Check for limits
	retries = (flags&E_BLOCKING_TXLIMIT) ? MAX_RETRY_COUNT_WHILE_BLOCKING : 1;
	while ( ((s32)octets > bytesLeftThisSecond) && --retries)
	{
        // do something !			
	}		
	if (retries==0) return BLOCKING_TXLIMIT_TIMEOUT;

	p.setLocalAddress( localAddress );

	/// Initialize Tx descriptor
	entry.packet = GETCOMPONENT( C_3c90xc ).kernelmem()->getMapping(&p);
	entry.packetLength = octets | (1<<31);		// indicate that buffer is last fragment
	entry.frameStartHeader = ADD_IP_CHECKSUM | DONT_ROUND_UP;   // could add packet ID
	bytesLeftThisSecond -= octets;

	/// Optionally, force polling here

	/// wait for completion if asked to do so (not very efficient...)
	if (flags&E_BLOCKING_TX) {
		retries = MAX_RETRY_COUNT_WHILE_BLOCKING;

		/// Note: how do I guarantee that this Tx is the packet I was trying to send?
		while ( !(READ8(TxStatus) & TxComplete)  && --retries)
		{

		}
		if (retries==0) return BLOCKING_TX_TIMEOUT;
	}
	return SUCCESS;
}


};	// namespace
