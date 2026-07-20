#ifndef PCNet32Driver_H
#define PCNet32Driver_H

#include "Drivers/CDriverBase.h"

#include "Drivers/PCI/CPCIDriver.h"


#include "Devices/Network/IEthernetDevice.h"

#include "MM/Virtual/ByteAllocator.h"


namespace MyOS { namespace Drivers { namespace Network { namespace PCNet32 {

/****
 * Dependencies
 ***/
using MyOS::Core::IComponent;
using MyOS::Drivers::IDriverManager;
using MM::IVirtualMemory;
using MM::IPhysicalMemory;
using InterruptHandling::IInterruptHandling;
using Devices::Network::IEthernetDevice;


class PCNet32Driver : public MyOS::Drivers::PCI::CPCIDriver {

// public is easier than declaring friends...
public:
	inline static const char* const ID() { return "d54a162a-6f34-4cd7-b507-4ecb01f1e256"; }

    IVirtualMemory* iVirtualMemory;
    IPhysicalMemory* iPhysicalMemory;
    IInterruptHandling* iInterruptHandling;
    
    PCNet32Driver(MyOS::Core::IContainer& container) throw() INITSECTION;
    virtual myos_result_t initialize(Context::IContext& context,
            NVPAIR params[]) INITSECTION;
    virtual myos_result_t deinitialize(Context::IContext& context);

    // CDriverBase
    virtual myos_result_t detectDevices() INITSECTION;

    // CPCIDriver
    virtual bool handleDevice(Drivers::PCI::IPCIDevice &dev);

private:
    static PCNet32Driver* instance;

public:
    inline static PCNet32Driver& getInstance()
    {
        return *instance;
    }

private:
    IEthernetDevice* impl1[1];

public:
    MM::ByteAllocator allocator;
};

// within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate(size_t s) throw(MM::OutOfMemoryException)
{
    return PCNet32Driver::getInstance().allocator.allocate(s );
}

inline void* allocateNoThrow(size_t s) throw()
{
    return PCNet32Driver::getInstance().allocator.allocateNoThrow(s );
}

inline void deallocate(void* p, size_t s)
{
    PCNet32Driver::getInstance().allocator.deallocate(p, s );
}
}
}
}
} // namespaces


#ifndef NEW_DECLARED
#define NEW_DECLARED

// Declared in global namespace
inline void* operator new(size_t size) throw(
        MyOS::MM::OutOfMemoryException)
{
    return MyOS::Drivers::Network::PCNet32::allocate(size);
}

inline void* operator new[](size_t size) throw(
        MyOS::MM::OutOfMemoryException)
{
    return MyOS::Drivers::Network::PCNet32::allocate(size);
}

inline void operator delete(void* m, size_t s)
{
    MyOS::Drivers::Network::PCNet32::deallocate(m, s);
}

#endif	// NEW_DECLARED

#endif
