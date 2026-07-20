/***************************************************************************
                          Dec21041Ethernet.cpp  -  description
                             -------------------
    begin                : Sat Oct 20 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include <new>
#include "Dec21041CIMPL.h"
#include "Dec21041Ethernet.h"
#include "EthernetPacket.h"
#include "Processor.h"
#include "IOPort.h"

namespace Dec21041 {

Dec21041Ethernet::Dec21041Ethernet( Dec21041Device &dev )
	: device(dev), maxBytesPerSecond(100000), status(UNDEFINED)
{

}

I_PCIDevice&	// virtual
Dec21041Ethernet::getDevice() const
{
	return device;
}

inline u32
Dec21041Ethernet::readRegister( CSRREGISTERS reg ) const
{
#ifdef DEC21041_MMIO
	return * (u32*) (&((u8*) ioBase)[reg]);
#else
	return inl( ioBase + reg );
#endif
}

inline void
Dec21041Ethernet::writeRegister( CSRREGISTERS reg, u32 value ) const
{
#ifdef DEC21041_MMIO	
	* (u32*) (&((u8*) ioBase)[reg]) = value;
#else
	outl( ioBase + reg, value );
#endif
}

void
Dec21041Ethernet::resetBusMode( u32 newMode )
{
	/// Reset, then wait specified 50 PCI cycles
	writeRegister( BUS_MODE_REG, 0x1 );
	GETCOMPONENT( DEC21041 ).timing()->delay( 100, Timing::us );
	writeRegister( BUS_MODE_REG, newMode );
	GETCOMPONENT( DEC21041 ).timing()->delay( 100, Timing::us );
}

inline void
Dec21041Ethernet::stopRxTx()
{
	u32 csr6 = readRegister( OPERATION_MODE_REG );
	if (csr6 & RxTx) {
		writeRegister( OPERATION_MODE_REG, csr6 & ~RxTx );
		readRegister( OPERATION_MODE_REG );			/// MMIO sync
	}
}

inline void
Dec21041Ethernet::startRxTx()
{
	u32 csr6 = readRegister( OPERATION_MODE_REG );
	if ((csr6 & RxTx) != RxTx) {
		writeRegister( OPERATION_MODE_REG, csr6 | RxTx );
		readRegister( OPERATION_MODE_REG );			/// MMIO sync
	}
}

bool
Dec21041Ethernet::initializeRings()
{
	PHYSICALADDRESS rxphys = dmaPhysical + OFFSET_OF( DMADATA, rxBuffers );

	/// wrap the rxring at the last entry
	RXTXDESCRIPTOR *desc = &dmadata->rxRing[RXRINGMASK];
	desc->length = PACKET_BUF_SIZE | DESC_RING_WRAP;		// this means: use CSR2/3 address to wrap
	desc->buffer1 = rxphys;

	// buffer2 used for linear address, chip (should) ignore it	since length==0 above
	desc->buffer2 = (u32) dmadata + OFFSET_OF( DMADATA, rxBuffers ) + PACKET_BUF_SIZE * RXRINGMASK;
	desc->status = DescOwned;

	u32 linadr = (u32) dmadata + OFFSET_OF( DMADATA, rxBuffers );
	for ( desc=dmadata->rxRing; desc < &dmadata->rxRing[RXRINGMASK]; ++desc, linadr += PACKET_BUF_SIZE )
	{
			desc->length = PACKET_BUF_SIZE;
      desc->buffer1 = (rxphys+=PACKET_BUF_SIZE);
			desc->buffer2 = linadr;
			desc->status = DescOwned;
	}

	/// clear the OWN bits in the TX ring descriptors, create a ring ?? (not really needed)
	memset( dmadata->txRing, 0, sizeof(dmadata->txRing) );
	return true;
}


bool
Dec21041Ethernet::setClient( Client& c, I_EthernetDevice **handle,
		u32 heartBeatPeriodInSeconds, RATELIMIT sendRateLimitBytesPerSecond /*=0*/ )
{
	if (client==0) {
		client = &c;
		if (sendRateLimitBytesPerSecond) maxBytesPerSecond = sendRateLimitBytesPerSecond;
		clientHeartbeatFrequency = heartBeatPeriodInSeconds;
		clienthandle = handle;
		return true;
	}
	return false;
}

void
Dec21041Ethernet::removeClient( Client &c )
{
	if (client==&c) {
		maxBytesPerSecond = 100000;
		client = 0;		
	}
}

#ifdef STATISTICS
s32
Dec21041Ethernet::statistics( PacketWriter &buf ) const
{
	return -1;
}
#endif

bool
Dec21041Ethernet::initialize( const PCIDEVICEINFO &info ) /* INITSECTION */
{
	GETCOMPONENT( DEC21041 ).display()->print( "\nDec21041Ethernet::initialize" );
#ifdef DEC21041_MMIO
	ioBase = (u32) info.MMIOstart;
#else
	ioBase = info.IObase;
#endif

	if (ioBase==0) return false;

	// Allocate DMA memory
	dmadata = (DMADATA*) GETCOMPONENT( DEC21041 ).allocate( sizeof(DMADATA), KM_DMA );
	if (!dmadata) return false;
	dmaPhysical = GETCOMPONENT( DEC21041 ).kernelmem()->getMapping(dmadata);

	// read physical address from EEPROM, but first determine address size
	u8 eepromAdrSize = readEEPROM( 0xff, 8 ) & 0x40000 ? 8 : 6;
	for (u32 i=0; i<sizeof(ETHERNETADDRESS)/2; ++i) {
  	(u16&) localAddress[2*i] = (u16) readEEPROM( i+10, eepromAdrSize );
	}	

	// TO DO: send init Tx frame

	status = INITIALIZED;
	return true;
}

void
Dec21041Ethernet::deinitialize()
{
	/// Check if device was initialized at all
	if (ioBase && status!=DEINITIALIZED) {
		stopRxTx();
		writeRegister( INTERRUPT_MASK_REG, 0 );
		if (dmadata) {
			GETCOMPONENT( DEC21041 ).free( dmadata, sizeof(DMADATA), KM_DMA );	
			dmadata = 0;
		}
		status = DEINITIALIZED;
	}
}

bool
Dec21041Ethernet::open( )
{
	if (status!=INITIALIZED && status!=CLOSED) return false;

	if (!initializeRings()) return false;
	
	/// 21041 does not have ACPI -> no set power state
	resetBusMode( 0x01A00000 | 0x8000 );	// taken from tulip_core.c

	/// Set addresses of rings
	writeRegister( RX_BASE, dmaPhysical );
	writeRegister( TX_BASE, dmaPhysical+OFFSET_OF(DMADATA, txRing) );

	writeRegister( SIA_CONNECTIVITY, 0x00000000 );
	writeRegister( SIA_TXRX, 0xFFFFFFFF );
	writeRegister( SIA_GENERAL, 0x00000008 );		// listen on AUI too
	writeRegister( OPERATION_MODE_REG, 0x80020000 | FullDuplex );
	writeRegister( SIA_CONNECTIVITY, csr13_eng | csr13_srl );

	// Start the chip's Tx to process setup frame
	stopRxTx();
	GETCOMPONENT( DEC21041 ).timing()->delay( 5, Timing::us );
	writeRegister( OPERATION_MODE_REG, 0x80020000 | FullDuplex | TxOn );

	/// Enable interrupts
	writeRegister( STATUS_REG, ALL_INTERRUPTS );
	writeRegister( INTERRUPT_MASK_REG, ALL_INTERRUPTS );
	startRxTx();

	// Do Rx poll demand
	writeRegister( RX_POLL_DEMAND, 0 );
	status = OPENED;
	return true;
}

void
Dec21041Ethernet::close()
{
	stopRxTx();
	status = CLOSED;
}

void
Dec21041Ethernet::handleError( u32 status )
{
	/// abnormal int is set in any case
	*GETCOMPONENT( DEC21041 ).display() << "\nhandleError "
	<< 	((status&SytemError) ? "system error" 	:
			(status&RxJabber) ? "RxJabber" :
			(status&RxDied) ? "RxDied" :
			(status&RxNoBuf) ? "RxNoBuf" :
			(status&TimerInt) ? "TimerInt" : "?" );
}

void
Dec21041Ethernet::handleIRQ()
{
	// *GETCOMPONENT( DEC21041 ).display() << "\nIRQ status=" << readRegister( STATUS_REG );
	u32 maxwork = MAXIRQWORK;

	register u32 csr5;
	while ( (csr5=readRegister( STATUS_REG )) & (NormalIntr|AbnormalIntr) && --maxwork )
	{
		/// ACK source ASAP
		writeRegister( STATUS_REG, csr5 & 0x0001ffff );
		if (csr5 & (RxIntr | RxNoBuf)) handleRX();
		if (csr5 & (TxNoBuf | TxDied | TxIntr)) handleTX();
		if (csr5 & AbnormalIntr) handleError(csr5);
		
		// TO DO: timer
	}
}

void
Dec21041Ethernet::handleRX()
{
	// *GETCOMPONENT( DEC21041 ).display() << "\nRX";
	u32 maxwork = MAXIRQWORK;		// or smaller...
	RXTXDESCRIPTOR *entry = &dmadata->rxRing[curRX & RXRINGMASK];
	register s32 status;
	while ( !((status = entry->status) & DescOwned) && maxwork-- )
	{
		/// Check if first+lastdescriptor is set, error is reset, and packet not too large ( <= 11 bits)
		if ( (status & (0x78000000 | RxWholePacket | ErrorSummary)) == RxWholePacket) {
      if (client) client->receive(
					*(EthernetPacket*) entry->buffer2,
				((status >> 16) & 0x7fff) - 4,					/// max. 15 bits, subtract 4 for CRC
				status & MulticastFrame ? Client::MULTICAST_MATCH : Client::PHYSICAL_MATCH
			);
		} else {
    	// count errors, perhaps reset chip ?
			*GETCOMPONENT( DEC21041 ).display() << "\nRX..errorstatus=" << (u32) status;
		}
		/// linux tulip driver uses 'refill_rx' allocating new buffers. I just use the old ones...
		entry->status = DescOwned;						
		entry = &dmadata->rxRing[++curRX & RXRINGMASK];
	}
}

void
Dec21041Ethernet::handleTX()
{
	*GETCOMPONENT( DEC21041 ).display() << "\nhandleTx";
}

I_EthernetDevice::RESULT
Dec21041Ethernet::transmit( EthernetPacket &p, size_t payload, u32 flags )
{
	// if (status!=S_OPENED) return DEVICE_NOT_OPEN;

	// include ethernet headersize
	payload += EthernetPacket::HEADERSIZE;

	RXTXDESCRIPTOR &entry = dmadata->txRing[ curTX.ReadAdd(1) & 0x3 ];
		
	// blocking wait here, no semaphore: this driver only provides the mechanism, not the policy
	u32 retry = (flags&E_BLOCKING_TXQUEUE) ? MAX_RETRY_COUNT_WHILE_BLOCKING : 1;
	while ( (entry.status & DescOwned) && --retry )
	{
        // do something useful here ? allocate memory perhaps ? could read timer to see how long there is!		
	};
	if (retry==0) return BLOCKING_TXQUEUE_TIMEOUT;
		
	// separate loop, to not continuously read the PCI register
	retry = (flags&E_BLOCKING_TXLIMIT) ? MAX_RETRY_COUNT_WHILE_BLOCKING : 1;
	while ((( payload > maxBytesPerSecond))
			&& --retry)
	{
        // do something !			
	}		
	if (retry==0) return BLOCKING_TXLIMIT_TIMEOUT;

	/// Fill in the local address
	p.setLocalAddress( localAddress );

	// get the physical address
	entry.buffer1 = GETCOMPONENT( DEC21041 ).kernelmem()->getMapping(&p);

  // Chip has auto-padding
	u32 flag = 0x60000000;	// no interrupt
	if (&entry==&dmadata->txRing[TXRINGMASK]) {
   	flag |= DESC_RING_WRAP;
	}
	entry.length = payload | flag;
	entry.status = DescOwned;

	// Trigger an immediate transmit demand
	writeRegister(TX_POLL_DEMAND,0);

  // wait for DMA completion (but dont keep polling PCI memory...)
  if ((flags&E_BLOCKING_TX) && Processor::InterruptsEnabled()) {
	  retry = MAX_RETRY_COUNT_WHILE_BLOCKING;
		while ((entry.status & DescOwned) && --retry)
		{
			// Note: this only works with interrupts *enabled* !!
		};
		if (retry==0) return BLOCKING_TX_TIMEOUT;
	}

	++packetsSent;
	bytesSent += payload;
	return SUCCESS;
}

u32
Dec21041Ethernet::readEEPROM( u32 location, u32 addr_len)
{
	u32 retval = 0;
	u32 read_cmd = location | (EE_READ_CMD << addr_len);

	writeRegister( BOOT_AND_SERIAL_ROM, EE_ENB & ~EE_CS );
	writeRegister( BOOT_AND_SERIAL_ROM, EE_ENB );

/// When using MMIO, regular IO is too fast to serve as delay
#ifdef DEC21041_MMIO
	/// p.191 of the documentation says a maximum of 250ns is required (sometimes less)
	#define eeprom_delay() GETCOMPONENT( DEC21041 ).timing()->delay( 250, Timing::ns );
#else
	#define eeprom_delay() readRegister( BOOT_AND_SERIAL_ROM )
#endif

	/* Shift the read command bits out. */
	for (s32 i = 4 + addr_len; i >= 0; i--) {
		short dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		writeRegister( BOOT_AND_SERIAL_ROM, EE_ENB | dataval );
		eeprom_delay();
		writeRegister( BOOT_AND_SERIAL_ROM, EE_ENB | dataval | EE_SHIFT_CLK );
		eeprom_delay();
		retval = (retval << 1) | ((readRegister(BOOT_AND_SERIAL_ROM) & EE_DATA_READ) ? 1 : 0);
	}
	writeRegister( BOOT_AND_SERIAL_ROM, EE_ENB );
	eeprom_delay();

	/// read another 16 bits, one at a time
	for (s32 i = 16; i > 0; i--) {
		writeRegister( BOOT_AND_SERIAL_ROM, EE_ENB | EE_SHIFT_CLK );
		eeprom_delay();
		retval = (retval << 1) | ((readRegister(BOOT_AND_SERIAL_ROM) & EE_DATA_READ) ? 1 : 0);
		writeRegister( BOOT_AND_SERIAL_ROM, EE_ENB );
		eeprom_delay();
	}

	// Terminate the EEPROM access
	writeRegister( BOOT_AND_SERIAL_ROM, EE_ENB & ~EE_CS );
	return retval;
}


};	// namespace
