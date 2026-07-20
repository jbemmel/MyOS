#ifndef IUSBRootHubImpl_H
#define IUSBRootHubImpl_H

#include "Devices/USB/IUSBRootHub.h"
#include "InterruptHandling/IInterruptHandling.h"
#include "MM/IVirtualMemory.h"
#include "MM/IPhysicalMemory.h"
#include "Devices/IOPort.h"
#include "USBIO.h"
#include "USBHubPort.h"
#include "Atomic/AtomicBitArray.h"

// for UHCIDriver
#include "MultiThreading/IMultiThreading.h"
#include "MultiThreading/Synchronization/CSyncPoint.h"

namespace MyOS { 
	
namespace Drivers {
    class IDriverManager;   
    namespace PCI { class IPCIDevice; }
}
using namespace Drivers::PCI;
using MultiThreading::IMultiThreading;
using MultiThreading::Synchronization::CSyncPoint;
	
namespace Devices { namespace USB {

   using Context::IContext;
   using MM::IVirtualMemory;
   using MM::IPhysicalMemory;
   
   using namespace InterruptHandling;

/**
 * Each root hub starts a thread to handle IRQs
 * There is a search for the right TD anyway, might as well do that in the thread
 * Needed for signalling and callbacks
 */
class IUSBRootHubImpl : public IUSBRootHub, public IInterruptContext,
    public MultiThreading::IRunnable
{

public:
    void* operator new( size_t );
    void operator delete( void* b );

    IUSBRootHubImpl( MyOS::Core::IComponent& c, IPCIDevice& pcidev ) INITSECTION;

    bool init( Drivers::IDriverManager& dm ) INITSECTION;
    void deinit( IContext& context );   

    // IInterruptContext
    virtual void onInterrupt( );

    // IRunnable
    virtual int run( NVPAIR params[] );

    // Blocking method to schedule a message to or from a device
    bool schedule( USB_CONTROL_PACKET& ctrl, IUSBDeviceImpl& d );    
    bool schedule( USB_CONTROL_PACKET& ctrl, IUSBDeviceImpl& d, void* data, size_t dataSize );
    
private:
	void createSetupPacket();

	inline void writeCommand( u16 cmd ) {
		outw( USBBasePort + E_CMD, cmd );	
	}
	inline u16 readCommand() {
		return inw( USBBasePort + E_CMD );	
	}
	inline u16 readStatus() {
		return inw( USBBasePort + E_STATUS );	
	}
	inline void clearStatus( u16 stat ) {
		outw( USBBasePort + E_STATUS, stat );	
	}
	inline void enableInterrupts( u16 lines ) {
		outw( USBBasePort + E_INTENA, lines );	
	}
    inline u16 curFrame() const {
        return inw( USBBasePort + E_FRAMENO );
    }
	inline void setFrame( u16 f ) {
		outw( USBBasePort + E_FRAMENO, f );
	}
	inline void setFrameListBase( u32 physAddr ) {
		outl( USBBasePort + E_FRAMELISTBASE, physAddr );
	}
 
    // Reads a USB port, n=0..x (usually 1 for 2 ports total)
	inline u16 readPort( u8 n ) {
		return inw( USBBasePort + E_PORT1SC + (n+n) );
	}

    inline void writePort( u8 n, u16 bits ) {
      outw( USBBasePort + E_PORT1SC + (n+n), bits );
    }


/*
 *  Technically, hubs can support up to N ports
 *  For now, I assume minimal two ports as mandated by the specs
 */
    USBHubPort port1, port2; 

	IPCIDevice& dev;
    USB_FLP* framelist;		// up to 1024 entries, indexed by E_FRAMENO, each represents 1 ms
    MM::physadr_t dmaMemory;
    
    // TODO: nice
    USB_TD* tdhack;

    // For allocating device addresses 1..127 (0 == default, reserved)
    AtomicBitArray<128> addresses;

    // Pre allocated data TDs, cannot be allocated on stack
    AtomicBitArray<4096/sizeof(USB_TD)> data_tds;
    USB_TD* tddata;
    MM::physadr_t dmaTddata;

    MultiThreading::Thread* thread;
    CSyncPoint sync;    // for now, perhaps better per TD
    volatile u32 waitpoint;
	u16 USBBasePort;
};
   
}
}} // namespaces
#endif
