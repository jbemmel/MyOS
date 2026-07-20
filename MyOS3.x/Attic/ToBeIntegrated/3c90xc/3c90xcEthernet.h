/***************************************************************************
                          C_3c90xcEthernet.h  -  description
                             -------------------
    begin                : Fri Oct 19 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef _C_3c90xcEthernet_H
#define _C_3c90xcEthernet_H

/// undef to get PortIO
#define _3c90xc_USE_MMIO

#include "Networking/Ethernet/EthernetPacket.h"
#include "3c90xcChip.h"
#include "Atomic/atomic32.h"

struct PCIDEVICEINFO;

namespace _3c90xc {

class C_3c90xcDevice;

using Ethernet::EthernetPacket;
using Ethernet::I_EthernetDevice;
using Ethernet::ETHERNETADDRESS;
using Ethernet::RATELIMIT;

class C_3c90xcEthernet : public I_EthernetDevice
{
public:
	C_3c90xcEthernet( C_3c90xcDevice &dev );
	virtual RESULT transmit( EthernetPacket &p, size_t octets, u32 flags );
	virtual ETHERNETADDRESS& getLocalAddress() const { return localAddress; }
	virtual bool setClient( Client& c, I_EthernetDevice **handle,
		u32 heartBeatPeriodInSeconds, RATELIMIT sendRateLimitBytesPerSecond /*=0*/ );
	virtual void removeClient( Client &c );
	virtual I_PCIDevice& getDevice() const;

#ifdef STATISTICS
	s32 statistics( PacketWriter &buf ) const;
#endif

	bool initialize( const PCIDEVICEINFO &info ) INITSECTION;
	void deinitialize();
	bool open();
	void close();
	void handleIRQ();

private:
	inline void reset();
	void initializeRings();

	void handleRX( u32 status );
	void handleTX();
	void handleError( u32 status );

	inline void globalReset();
	void TXReset();
	void RXReset( u32 resetbits );

	void readEEPROM();
	void delay( u32 ns );

	inline void selectWindow( u8 win );

	C_3c90xcDevice &device;

	// RX status: current descriptor in the RX ring
	u32 curRX;
	atomic32 curTX;

	// Client data
	I_EthernetDevice::Client *client;
	RATELIMIT maxBytesPerSecond;
	u32 clientHeartbeatFrequency;
	I_EthernetDevice **clienthandle;
	s32 bytesLeftThisSecond;

	enum {
		LOG2RXRINGSIZE = 5, RXRINGMASK = (1<<LOG2RXRINGSIZE)-1,
		LOG2TXRINGSIZE = 4, TXRINGMASK = (1<<LOG2TXRINGSIZE)-1,
		MAXIRQWORK = 20,
		PACKET_BUF_SIZE = 1528,
		MAX_RETRY_COUNT_WHILE_BLOCKING = 100000000,	// max. forloop count
		NO_LIMIT = 100000000		// > 100 Mbps
	};

	/// Structure in DMA memory
	struct DMADATA {
		TXDESCRIPTOR txRing[ (1<<LOG2TXRINGSIZE) ];		// must be aligned on 8-byte boundaries !
		RXDESCRIPTOR rxRing[ (1<<LOG2RXRINGSIZE) ];
	} *dmadata;

	/// Linear addresses, in normal memory
	LINEARADDRESS txAddresses[(1<<LOG2TXRINGSIZE)];

	u32 dmaPhysicalAddress;		// physical address of config
	u32 chipID;
	enum STATE { UNDEFINED = 0, INITIALIZED, OPENED, CLOSED, DESTRUCTED } status;

	u32 IObase;		// used for both MMIO and PORTIO
	ETHERNETADDRESS localAddress;
	u16 eepromData[0x20];		// see p. 71 (Table 11) for layout, there are more words to read
	volatile bool timerFired;
};

};	// namespace

#endif
