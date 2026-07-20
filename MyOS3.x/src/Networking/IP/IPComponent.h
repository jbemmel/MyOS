#ifndef IPComponent_H
#define IPComponent_H

#include "Core/IComponent.h"
#include "interfaces.h"

#include "Networking/IP/IIPLayerImpl.h"
#include "MM/Virtual/ByteAllocator.h"


namespace MyOS { namespace Networking { namespace IP {

// Dependencies
using MyOS::Core::IComponent;
using MM::IVirtualMemory;
using MultiThreading::IMultiThreading;
using Timer::ITimerFacility;

class IPComponent : public IComponent
{
public:
    // easier
    IVirtualMemory* iVirtualMemory;
    IMultiThreading* iMultiThreading;
    ITimerFacility* iTiming;

public:
    inline static const char* const ID()
    {
        return "2525e92e-c8b5-45b6-8e30-09ce70d8e4de";
    }

    IPComponent(MyOS::Core::IContainer& container) throw() INITSECTION;
    virtual myos_result_t initialize(Context::IContext& context,
            NVPAIR params[]) INITSECTION;
    virtual myos_result_t deinitialize(Context::IContext& context);

    virtual myos_result_t queryInterface(IComponent& requestor,
            const uuid_t& uuid, IInterface*& result);

private:
    static IPComponent* instance;
public:
    inline static IPComponent& getInstance()
    {
        return *instance;
    }

private:

    friend class IIPLayerImpl; // access interfaces
    IIPLayerImpl impl1;

public:
    MM::Allocator allocator;

};

// within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate(size_t s) throw(MM::OutOfMemoryException)
{
    return IPComponent::getInstance().allocator.allocateAutoSize(s );
}

inline void* allocateNoThrow(size_t s) throw()
{
    return IPComponent::getInstance().allocator.allocateAutoSizeNoThrow(s);
}

inline void deallocate(void* p)
{
    IPComponent::getInstance().allocator.freeAutoSize(p);
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
    return MyOS::Networking::IP::allocate(size);
}

inline void* operator new[](size_t size) throw(
        MyOS::MM::OutOfMemoryException)
{
    return MyOS::Networking::IP::allocate(size);
}

inline void operator delete(void* m)
{
    MyOS::Networking::IP::deallocate(m);
}

#endif	// NEW_DECLARED

#endif
