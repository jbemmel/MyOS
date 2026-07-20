#ifndef FloppyDriver_H
#define FloppyDriver_H

#include "Drivers/CDriverBase.h"
#include "Devices/Floppy/IFloppy.h"
#include "MM/Virtual/ByteAllocator.h"
#include "interfaces.h"

namespace MyOS { namespace Drivers { namespace Floppy { namespace Default {

/****
 * Dependencies
 ***/
using MyOS::Core::IComponent;
using MyOS::Drivers::IDriverManager;
//using MyOS::MM:Virtual::IVirtualMemory;
using InterruptHandling::IInterruptHandling;
using Timer::ITimerFacility;
using Devices::Floppy::IFloppy;

class FloppyDriver : public MyOS::Drivers::CDriverBase
{

    // public is easier than declaring friends...
public:
    inline static const char* const ID() { return "882dfb4e-bd23-4f36-9146-39db3ef7c50d"; }
    
    IVirtualMemory* iVirtualMemory;
    IInterruptHandling* iInterruptHandling;
    ITimerFacility* iTiming;
    
    FloppyDriver(MyOS::Core::IContainer& container) throw() INITSECTION;
    virtual myos_result_t initialize(Context::IContext& context,
            NVPAIR params[]) INITSECTION;
    virtual myos_result_t deinitialize(Context::IContext& context);

    // CDriverBase
    virtual myos_result_t detectDevices() INITSECTION;

private:
    static FloppyDriver* instance;

public:
    inline static FloppyDriver& getInstance()
    {
        return *instance;
    }

private:
    friend class FloppyFSM;
    IFloppy* impl1[1];

public:
    MM::ByteAllocator allocator;

};

// within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate(size_t s) throw(MM::OutOfMemoryException)
{
    return FloppyDriver::getInstance().allocator.allocate(s );
}

inline void* allocateNoThrow(size_t s) throw()
{
    return FloppyDriver::getInstance().allocator.allocateNoThrow(s );
}

inline void deallocate(void* p, size_t s)
{
    FloppyDriver::getInstance().allocator.deallocate(p, s );
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
    return MyOS::Drivers::Floppy::Default::allocate(size);
}

inline void* operator new[](size_t size) throw(
        MyOS::MM::OutOfMemoryException)
{
    return MyOS::Drivers::Floppy::Default::allocate(size);
}

inline void operator delete(void* m, size_t s)
{
    MyOS::Drivers::Floppy::Default::deallocate(m, s);
}

#endif	// NEW_DECLARED

#endif
