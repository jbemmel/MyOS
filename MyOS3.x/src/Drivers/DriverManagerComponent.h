#ifndef DriverManagerComponent_H
#define DriverManagerComponent_H

#include "Core/IComponent.h"

#include "Drivers/IDriverManagerImpl.h"

#include "MM/Virtual/ByteAllocator.h"


namespace MyOS
{
namespace Drivers
{

/****
 * Dependencies
 ***/
using MyOS::Core::IComponent;
using MM::IVirtualMemory;
using InterruptHandling::IInterruptHandling;


class DriverManagerComponent : public IComponent
{
public:
    // easier
    IVirtualMemory* iVirtualMemory;
    IInterruptHandling* iInterruptHandling;

public:
    inline static const char* const ID()
    {
        return "c8a70b00-5f59-11d6-9e80-0010a708e02f";
    }

    DriverManagerComponent(MyOS::Core::IContainer& container) throw() INITSECTION;
    virtual myos_result_t initialize(Context::IContext& context,
            NVPAIR params[]) INITSECTION;
    virtual myos_result_t deinitialize(Context::IContext& context);

    virtual myos_result_t queryInterface(IComponent& requestor,
            const uuid_t& uuid, IInterface*& result);

private:
    static DriverManagerComponent* instance;
public:
    inline static DriverManagerComponent& getInstance()
    {
        return *instance;
    }

private:

    friend class IDriverManagerImpl; // access interfaces
    friend class PCI::IPCISupportImpl;
    IDriverManagerImpl impl1;

public:
    MM::Allocator allocator;

};

// within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate(size_t s) throw(MM::OutOfMemoryException)
{
    return DriverManagerComponent::getInstance().allocator.allocateAutoSize(s );
}

inline void* allocateNoThrow(size_t s) throw()
{
    return DriverManagerComponent::getInstance().allocator.allocateAutoSizeNoThrow(s );
}

inline void deallocate(void* p)
{
    DriverManagerComponent::getInstance().allocator.freeAutoSize(p);
}

}
} // namespaces


#ifndef NEW_DECLARED
#define NEW_DECLARED

// Declared in global namespace
inline void* operator new(size_t size) throw(MyOS::MM::OutOfMemoryException)
{
    return MyOS::Drivers::allocate(size);
}

inline void* operator new[](size_t size) throw(MyOS::MM::OutOfMemoryException)
{
    return MyOS::Drivers::allocate(size);
}

inline void operator delete(void* m)
{
    MyOS::Drivers::deallocate(m);
}

#endif	// NEW_DECLARED

#endif
