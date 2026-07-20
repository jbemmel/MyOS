#ifndef NE2000Driver_H
#define NE2000Driver_H

#include "Drivers/CDriverBase.h"

#include "Devices/Network/IEthernetDevice.h"

#include "MM/Virtual/ByteAllocator.h"

namespace MyOS
{
namespace Drivers
{
namespace Network
{
namespace NE2000
{

/****
 * Dependencies
 ***/
using MyOS::Core::IComponent;
using MyOS::Drivers::IDriverManager;
using MM::IVirtualMemory;
using InterruptHandling::IInterruptHandling;
using MultiThreading::IMultiThreading;
using Devices::Network::IEthernetDevice;

class NE2000Driver : public MyOS::Drivers::CDriverBase
{

    // public is easier than declaring friends...
public:
    inline static const char* const ID()
    {
        return "d54a162a-6f34-4cd7-b507-4ecb01f1e256";
    }

    IVirtualMemory* iVirtualMemory;
    IInterruptHandling* iInterruptHandling;
    IMultiThreading* iMultiThreading;
    
    NE2000Driver(MyOS::Core::IContainer& container) throw() INITSECTION;

    virtual myos_result_t initialize(Context::IContext& context,
            NVPAIR params[]) INITSECTION;
    virtual myos_result_t deinitialize(Context::IContext& context);

    // CDriverBase
    virtual myos_result_t detectDevices() INITSECTION;

private:
    static NE2000Driver* instance;

public:
    inline static NE2000Driver& getInstance()
    {
        return *instance;
    }

private:
    IEthernetDevice* impl1[1];

public:
    MM::Allocator allocator;

};

// within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate(size_t s) throw(MM::OutOfMemoryException)
{
    return NE2000Driver::getInstance().allocator.allocateAutoSize(s );
}

inline void* allocateNoThrow(size_t s) throw()
{
    return NE2000Driver::getInstance().allocator.allocateAutoSizeNoThrow(s );
}

inline void deallocate(void* p)
{
    NE2000Driver::getInstance().allocator.freeAutoSize(p );
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
    return MyOS::Drivers::Network::NE2000::allocate(size);
}

inline void* operator new[](size_t size) throw(
        MyOS::MM::OutOfMemoryException)
{
    return MyOS::Drivers::Network::NE2000::allocate(size);
}

inline void operator delete(void* m)
{
    MyOS::Drivers::Network::NE2000::deallocate(m);
}

#endif	// NEW_DECLARED

#endif
