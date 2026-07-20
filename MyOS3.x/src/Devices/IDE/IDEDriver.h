#ifndef IDEDriver_H
#define IDEDriver_H

#include "Drivers/CDriverBase.h"
#include "IIDEManager.h"
#include "MM/Virtual/ByteAllocator.h"

namespace MyOS
{
namespace Drivers
{
namespace IDE
{

class IIDEManager;

namespace Default
{

/****
 * Dependencies
 ***/
using MyOS::Core::IComponent;
using MyOS::Drivers::IDriverManager;
using MM::IVirtualMemory;
using InterruptHandling::IInterruptHandling;
using MultiThreading::IMultiThreading;

class IDEDriver : public MyOS::Drivers::CDriverBase
{

    // public is easier than declaring friends...
public:
    inline static const char* const ID()
    {
        return "0e639e52-7800-4aa2-8a57-9a3c9e05d320";
    }

    IVirtualMemory* iVirtualMemory;
    IInterruptHandling* iInterruptHandling;
    IMultiThreading* iMultiThreading;
    IDEDriver(MyOS::Core::IContainer& container) throw() INITSECTION;
    virtual myos_result_t initialize(Context::IContext& context,
            NVPAIR params[]) INITSECTION;
    virtual myos_result_t deinitialize(Context::IContext& context);

    // CDriverBase
    virtual myos_result_t detectDevices() INITSECTION;

private:
    static IDEDriver* instance;

public:
    inline static IDEDriver& getInstance()
    {
        return *instance;
    }

private:
    IIDEManager* impl1[1];

public:
    MM::ByteAllocator allocator;

};

// within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate(size_t s) throw(MM::OutOfMemoryException)
{
    return IDEDriver::getInstance().allocator.allocate(s );
}

inline void* allocateNoThrow(size_t s) throw()
{
    return IDEDriver::getInstance().allocator.allocateNoThrow(s );
}

inline void deallocate(void* p, size_t s)
{
    IDEDriver::getInstance().allocator.deallocate(p, s );
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
    return MyOS::Drivers::IDE::Default::allocate(size);
}

inline void* operator new[](size_t size) throw(
        MyOS::MM::OutOfMemoryException)
{
    return MyOS::Drivers::IDE::Default::allocate(size);
}

inline void operator delete(void* m, size_t s)
{
    MyOS::Drivers::IDE::Default::deallocate(m, s);
}

#endif	// NEW_DECLARED

#endif
