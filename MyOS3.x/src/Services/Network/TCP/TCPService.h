#ifndef TCPService_H
#define TCPService_H

#include "Core/IService.h"
#include "interfaces.h"

#include "MM/Virtual/ByteAllocator.h"

namespace MyOS
{
namespace Services
{
namespace Network
{
namespace TCP
{

// Dependencies
using MyOS::Core::IService;
using MM::IVirtualMemory;
using MultiThreading::IMultiThreading;
using Timer::ITimerFacility;
using Networking::IP::IIPLayer;

class TCPService : public IService
{
public:
    // easier
    IVirtualMemory* iVirtualMemory;
    IMultiThreading* iMultiThreading;
    ITimerFacility* iTiming;
    IIPLayer* iIPLayer;

public:
    inline static const char* const ID()
    {
        return "99d5be5c-57bc-48e0-ad35-c67d97a32cf0";
    }

    TCPService(MyOS::Core::IContainer& container) throw() INITSECTION;
    virtual myos_result_t initialize(Context::IContext& context, NVPAIR params[]) INITSECTION;
    virtual myos_result_t deinitialize(Context::IContext& context);

private:
    static TCPService* instance;
public:
    inline static TCPService& getInstance()
    {
        return *instance;
    }

private:

    // service routine
    virtual bool onServicePointCreated(void* servicePoint);

public:
    MM::Allocator allocator;

};

// within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate(size_t s) throw(MM::OutOfMemoryException)
{
    return TCPService::getInstance().allocator.allocateAutoSize(s);
}

inline void* allocateNoThrow(size_t s) throw()
{
    return TCPService::getInstance().allocator.allocateAutoSizeNoThrow(s);
}

inline void deallocate(void* p)
{
    TCPService::getInstance().allocator.freeAutoSize(p);
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
    return MyOS::Services::Network::TCP::allocate(size);
}

inline void* operator new[](size_t size) throw(
        MyOS::MM::OutOfMemoryException)
{
    return MyOS::Services::Network::TCP::allocate(size);
}

inline void operator delete(void* m)
{
    MyOS::Services::Network::TCP::deallocate(m);
}

#endif	// NEW_DECLARED

#endif
