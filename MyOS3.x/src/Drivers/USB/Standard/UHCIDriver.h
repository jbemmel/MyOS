#ifndef UHCIDriver_H
#define UHCIDriver_H

#include "Drivers/CDriverBase.h"

#include "Drivers/PCI/CPCIDriver.h"


#include "Devices/USB/IUSBRootHub.h"

#include "MM/Virtual/ByteAllocator.h"


namespace MyOS { namespace Drivers { namespace USB { namespace Standard {

/****
 * Dependencies
 ***/
using MyOS::Core::IComponent;
using MyOS::Drivers::IDriverManager;
using MM::IVirtualMemory;
using MM::IPhysicalMemory;
using MultiThreading::IMultiThreading;
using Devices::USB::IUSBRootHub;

class UHCIDriver : public MyOS::Drivers::PCI::CPCIDriver {

// public is easier than declaring friends...
public:
	inline static const char* const ID() { return "982dfb4e-bd23-4f36-9146-39db3ef7c50e"; }

IVirtualMemory* iVirtualMemory;
IPhysicalMemory* iPhysicalMemory;
IMultiThreading* iMultiThreading;
UHCIDriver( MyOS::Core::IContainer& container ) throw() INITSECTION;
    virtual myos_result_t initialize( Context::IContext& context, NVPAIR params[] ) INITSECTION;
    virtual myos_result_t deinitialize( Context::IContext& context );

	// CDriverBase
    virtual myos_result_t detectDevices() INITSECTION;


	// CPCIDriver
	virtual bool handleDevice( Drivers::PCI::IPCIDevice &dev );


private:
    static UHCIDriver* instance;
    
public: inline static UHCIDriver& getInstance() { return *instance; }

private:    
IUSBRootHub* impl1[1];

	public: MM::ByteAllocator allocator;


};


// within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate( size_t s ) throw (MM::OutOfMemoryException) {
   return UHCIDriver::getInstance().allocator.allocate( s );
}

inline void* allocateNoThrow( size_t s ) throw () {
   return UHCIDriver::getInstance().allocator.allocateNoThrow( s );
}

inline void deallocate( void* p, size_t s ) {
   UHCIDriver::getInstance().allocator.deallocate( p, s );
}
}}
}} // namespaces



#ifndef NEW_DECLARED
#define NEW_DECLARED

// Declared in global namespace
inline void* operator new( size_t size ) throw (MyOS::MM::OutOfMemoryException) {
	return MyOS::Drivers::USB::Standard::allocate(size);  
}

inline void* operator new[]( size_t size ) throw (MyOS::MM::OutOfMemoryException) {
	return MyOS::Drivers::USB::Standard::allocate(size);  
}

inline void operator delete( void* m, size_t s ) {
	MyOS::Drivers::USB::Standard::deallocate(m,s);
}

#endif	// NEW_DECLARED



#endif
