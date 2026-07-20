#ifndef DHCPClientService_H
#define DHCPClientService_H

#include "IService.h"
#include "interfaces.h"
#include "MM/ByteAllocator.h"
#include "IIPLayer.h"

namespace MyOS { namespace Services { namespace Network { namespace DHCP {

using MyOS::Core::IService;

using MM::IVirtualMemory;

using Timer::ITimerFacility;

using MultiThreading::IMultiThreading;

using Networking::IP::IIPLayer;



class DHCPClientService : public IService
{
public:
    // easier
    IVirtualMemory* iVirtualMemory;
    ITimerFacility* iTiming;
    IMultiThreading* iMultiThreading;
    IIPLayer* iIPLayer;

public:
    inline static const char* const ID()
    {
        return "7e651f6c-9c74-4d02-99d9-edbe3e6bd852";
    }

    DHCPClientService(MyOS::Core::IContainer& container) throw() INITSECTION;
    virtual myos_result_t initialize(Context::IContext& context,
            NVPAIR params[]) INITSECTION;
    virtual myos_result_t deinitialize(Context::IContext& context);

private:
    static DHCPClientService* instance;
public:
    inline static DHCPClientService& getInstance()
    {
        return *instance;
    }

private:

    // service routine
    virtual bool onServicePointCreated(void* servicePoint);

public:
    MM::ByteAllocator allocator;

};

// within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate(size_t s) throw(MM::OutOfMemoryException)
{
    return DHCPClientService::getInstance().allocator.allocate(s );
}

inline void* allocateNoThrow(size_t s) throw()
{
    return DHCPClientService::getInstance().allocator.allocateNoThrow(s );
}

inline void deallocate(void* p, size_t s)
{
    DHCPClientService::getInstance().allocator.deallocate(p, s );
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
    return MyOS::Services::Network::DHCP::allocate(size);
}

inline void* operator new[](size_t size) throw(
        MyOS::MM::OutOfMemoryException)
{
    return MyOS::Services::Network::DHCP::allocate(size);
}

inline void operator delete(void* m, size_t s)
{
    MyOS::Services::Network::DHCP::deallocate(m, s);
}

#endif	// NEW_DECLARED

#endif
