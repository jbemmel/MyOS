/***************************************************************************
                          PCNet32Device.h  -  description
                             -------------------
    begin                : Thu Oct 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef PCNET32DEVICE_H
#define PCNET32DEVICE_H

#include "Drivers/PCI/IPCISupport.h"
#include "Devices/Network/IEthernetDevice.h"
#include "PCNet32Chip.h"
#include "Atomic/atomic32.h"

namespace MyOS { namespace Devices { namespace Network { namespace PCNet32 {

using namespace Drivers::PCI;

class PCNet32Device : public IEthernetDevice, public IInterruptContext
{
public:
	PCNet32Device( MyOS::Core::IComponent& c, IPCIDevice& dev );
	bool initialize( IPCISupport& p ) INITSECTION;
	void deinitialize();

	// IPacketDevice
	virtual myos_result_t open( IPacketReceiver& rx, u32 mode );
	virtual void close();
	virtual myos_result_t sendPacket( 
		const buffer& buf, u32 flags, IPacketSender* onDone, void* act );

	// IEthernetDevice, should be in base class
	virtual myos_result_t getAddress( IO::OStream& out ) const;

/*
	PCNet32Device( IPCIDriver &driver );
	bool initialize( IUnhandledPCIDevice& dev ) INITSECTION;
	void deinitialize();

	// I_PCIDevice
	virtual IUnhandledPCIDevice& getDevice() const { return *device; }
	virtual bool open(  params?  );
	virtual bool close(  params?  );
	virtual bool suspend();
	virtual bool resume();
	virtual IPCIDriver& getDriver() const { return driver; }
	virtual IInterface& getFunctionality() { return ethernet; }
	// virtual s32 list( PacketWriter& buf, u32 closeTag  ) const;
	// virtual s32 get( const char* callarguments, PacketWriter& buf );
	virtual const char* getInstanceID( char assign );

   // IInterface overrides
   virtual myos_result_t put( myos_call_t call, NVPAIR parameters[], IO::IStream& input );

private:
	const char instanceID[6];
	IPCIDriver &driver;
	IUnhandledPCIDevice* device;
	PCNet32Ethernet ethernet;
*/	
private:
	// IInterruptContext
	virtual void onInterrupt();
	void handleRX();
	void handleTX(u32);
	void handleError( u32 csr0 );
		
	void initializeRings() INITSECTION;	// ??

	inline u16 readCSR( CSR_REGISTERS index ) const;
	inline void writeCSR( CSR_REGISTERS index, u16 value ) const;
	inline u16 readBCR( BCR_REGISTERS index ) const;
	inline void writeBCR( BCR_REGISTERS index, u16 value );
	inline u16 readRAP() const;
	inline void writeRAP( u16 value ) const;
	inline void reset();
	inline void switchTo32bit();

	IPCIDevice& device;		// to be able to release it again
	u32 IObase;				// used for both MMIO and PORTIO
   
	// RX status: current descriptor in the RX ring
	u32 curRX, dirtyTX;
	atomic32 curTX;

	IPacketReceiver* rxclient;

	physadr_t dmaPhysicalAddress;		// physical address of config
	u32 chipID;
	enum STATE { UNDEFINED = 0, INITIALIZED, OPENED, CLOSED, DESTRUCTED } status;

	enum {
		LOG2RXRINGSIZE = 5, RXRINGMASK = (1<<LOG2RXRINGSIZE)-1,
		LOG2TXRINGSIZE = 5, TXRINGMASK = (1<<LOG2TXRINGSIZE)-1,
		MAXIRQWORK = 20,
		PACKET_BUF_SIZE = 1520,  	// should be multiple of 16 > (1500 + 14 + 4)
	};

	/// Structure in DMA-able memory of chip (but X86 is cache coherent)
	/// size = 16*2^5 + 16*2^5 + 1520*2^5 + 7*4 = 49692 bytes (12+ pages!)
	struct DMADATA {
		RXDESCRIPTOR rxRing[ 1 << LOG2RXRINGSIZE ];	// must(!) be 16-byte aligned
		TXDESCRIPTOR txRing[ 1 << LOG2TXRINGSIZE ];
		u8 rxBuffers[ PACKET_BUF_SIZE * (1 << LOG2RXRINGSIZE) ];
		CHIPCONFIG config;
	} *dmadata;   

	// Data maintained for each TX request, desc.userspace contains index
	struct TXDATA {
		u8*				payload;	// size is in desc.
		IPacketSender*  sender;		// optional
		void*			act;		// ACT token set by client
	} txdata[ 1 << LOG2TXRINGSIZE ];

	// work arounds for offsetof problem
	inline static size_t rxRingOfs() {
		return 0;
	}
	inline static size_t txRingOfs() {
		return sizeof(RXDESCRIPTOR[1 << LOG2RXRINGSIZE]);
	}
	inline static size_t rxBuffersOfs() {
		return txRingOfs()+sizeof(TXDESCRIPTOR[1 << LOG2TXRINGSIZE]);
	}
	inline static size_t configOfs() {
		return rxBuffersOfs()+PACKET_BUF_SIZE * (1 << LOG2RXRINGSIZE);
	}
	
#ifdef STATISTICS
	// counters
	u32 RXmissed, packetsSent, packetsReceived, RxIRQCalled;
	u32 bytesSent, bytesReceived, RXerrors, RXrecursion;
	// mutable u32 timerTicks, lastMeasureTick, lastMeasureOctets;
#endif
	
};

}}}}	// namespace

#endif
