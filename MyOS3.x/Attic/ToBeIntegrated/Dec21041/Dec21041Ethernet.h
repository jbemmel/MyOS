/***************************************************************************
                          Dec21041Ethernet.h  -  description
                             -------------------
    begin                : Fri Oct 19 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef Dec21041ETHERNET_H
#define Dec21041ETHERNET_H

#include "Ethernet.h"
#include "Dec21041Chip.h"
#include "atomic32.h"

/// undef to use PIO
#define DEC21041_MMIO

struct PCIDEVICEINFO;

namespace Dec21041 {

class Dec21041Device;

using Ethernet::EthernetPacket;
using Ethernet::I_EthernetDevice;
using Ethernet::ETHERNETADDRESS;
using Ethernet::RATELIMIT;

class Dec21041Ethernet : public I_EthernetDevice
{
public:
	Dec21041Ethernet( Dec21041Device &dev );
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
	void handleRX();
	void handleTX();
	void handleError( u32 csr0 );
	bool initializeRings();

	inline u32 readRegister( CSRREGISTERS reg ) const;
	inline void writeRegister( CSRREGISTERS reg, u32 value ) const;
	void resetBusMode( u32 newMode );
	u32 readEEPROM( u32 location, u32 addrLength /* 6 or 8 */ );
	inline void stopRxTx();
	inline void startRxTx();

	Dec21041Device& device;
	u32 ioBase;

	// RX status: current descriptor in the RX ring
	u32 curRX;
	atomic32 curTX;

	// client data
	I_EthernetDevice::Client *client;
	RATELIMIT maxBytesPerSecond;
	u32 clientHeartbeatFrequency;
	I_EthernetDevice **clienthandle;

	s32 bytesLeftThisSecond;

	// statistics
	u32 packetsSent, bytesSent;

	enum {
		LOG2RXRINGSIZE = 5, RXRINGMASK = (1<<LOG2RXRINGSIZE)-1,
		LOG2TXRINGSIZE = 4, TXRINGMASK = (1<<LOG2TXRINGSIZE)-1,
		MAXIRQWORK = 20,
		PACKET_BUF_SIZE = 1536,
		MAX_RETRY_COUNT_WHILE_BLOCKING = 100000000,	// max. forloop count

		/// fixed values
		EEPROM_SIZE = 128		// bytes
	};

	struct DMADATA {
		RXTXDESCRIPTOR rxRing[ 1 << LOG2RXRINGSIZE ];
		RXTXDESCRIPTOR txRing[ 1 << LOG2TXRINGSIZE ];
		u8 rxBuffers[ PACKET_BUF_SIZE * (1 << LOG2RXRINGSIZE) ];
	} *dmadata;
	PHYSICALADDRESS dmaPhysical;
	enum STATUS { UNDEFINED=0, INITIALIZED, OPENED, CLOSED, DEINITIALIZED } status;
	ETHERNETADDRESS localAddress;
};

};	// namespace

#endif
