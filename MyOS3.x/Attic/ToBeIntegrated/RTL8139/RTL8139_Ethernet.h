/***************************************************************************
                          RTL8139_Ethernet.h  -  description
                             -------------------
    begin                : Sun Aug 19 2001
    copyright            : (C) 2001 by jeroen
    email                : jeroen@thebem
 ***************************************************************************/
#ifndef RTL8139ETHERNET_H
#define RTL8139ETHERNET_H

#include "Networking/Ethernet/EthernetPacket.h"

// implementation headers
#include "Atomic/atomic32.h"

namespace RTL8139 {

class C_RTL8139_PCIDevice;

using Ethernet::EthernetPacket;
using Ethernet::I_EthernetDevice;
using Ethernet::ETHERNETADDRESS;
using Ethernet::RATELIMIT;

struct MMIO;

/**
 *  This class handles normal operation: buffer management, handling interrupts and
 *  transmissions
 *
 *  Normally, there is one instance per physical device
 */
class C_RTL8139_Ethernet : public I_EthernetDevice
{
public:
  /// Constructor, cannot have parameters when used in a component
  C_RTL8139_Ethernet( C_RTL8139_PCIDevice &dev );
  virtual ~C_RTL8139_Ethernet() {}

  bool initialize( LINEARADDRESS MMIOstart );
  bool takeOver( C_RTL8139_Ethernet &from );
  void deinitialize();
  bool resume();
	void close();

	// I_EthernetDevice
	virtual RESULT transmit( EthernetPacket &p, size_t octets, u32 flags );
  virtual ETHERNETADDRESS& getLocalAddress() const;
	virtual bool setClient( Client& c, I_EthernetDevice **handle,
		u32 heartBeatPeriodInSeconds, RATELIMIT sendRateLimitBytesPerSecond /*=0*/ );
	virtual void removeClient( Client &c );
	virtual I_PCIDevice& getDevice() const;

#ifdef STATISTICS
	s32 statistics( PacketWriter &buf ) const;
#endif

	void handleIRQ();
private:
/******************
    IMPLEMENTATION
******************/
	void chipreset();

	enum {
		MAX_WORK_PER_IRQ = 20,                /// Max. number of packets handled per IRQ
		TIMER_TICKS_PER_SECOND = 33000000,		/// assumes PCI @ 33Mhz
		
		/// max for-count while waiting for DMA to finish, 100 mljn
		MAX_RETRY_COUNT_WHILE_BLOCKING = 100000000,
		ENFORCE_MIN_PACKETSIZE = true,				/// Pad packets that are too short (<60 bytes)
		NO_LIMIT = 100000000									// bytes per second, should be > 100 Mbps
	};
	
	/// Non-static versions
	void handleRX( u32 status );
	void handleTX( u32 status );
	void handleError( u32 status, int linkChanged );	

	C_RTL8139_PCIDevice &device;

	/// Reference to the client
	I_EthernetDevice::Client* client;

	/// Reference to the chip
	volatile MMIO* chip;

	/// Reference to the rxBuffer
	u8* rxBuffer;

	/// The local ethernet address, bytes 0..5 are valid
	ETHERNETADDRESS localAddress;

	/// current index into rxBuffer, as acknowledged to the chip
	u16 curRx;

#ifdef STATISTICS    	
	// counters
	u32 RXmissed, packetsSent, packetsReceived, RxIRQCalled;
	u32 bytesSent, bytesReceived, RXerrors, RXrecursion;
#endif

	/// Client data
	s32 bytesPerSecondLimit;
	I_EthernetDevice **clienthandle;
	u32 clientHeartbeatFrequency;

	volatile s32 bytesAllowedThisSecond;	
	
	// cache for the 4 packet descriptors
	struct PACKETCACHE
	{
		volatile ATOMIC32<EthernetPacket*> packet;		// perhaps more: TxTime, ...
		bool notify : 1;
		size_t octets : 15;
	} PACKED packetcache[4];

  /// Counter for the next Tx buffer
  atomic32 packetsQueued;

    enum STATUS {
        S_UNDEFINED = 0,
        S_INITIALIZING,
        S_INITIALIZED,
        S_OPENED,
				S_CLOSED,
				S_DEINITIALIZED_WAS_OPEN,
				S_DEINITIALIZED_WAS_CLOSED,
    } status;		

	volatile bool linkUp;
};

};  // namespace

#endif
