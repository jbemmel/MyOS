#ifndef FATComponent_H
#define FATComponent_H

#include "Core/IComponent.h"

#include "FS/FAT/IFATFSImpl.h"

#include "MM/Virtual/ByteAllocator.h"

namespace MyOS
{
namespace FS
{
namespace FAT
{

/****
 * Dependencies
 ***/
using MyOS::Core::IComponent;
using MM::IVirtualMemory;
using MM::Paging::IDemandPaging;

class FATComponent : public IComponent
{
public:
    // easier
    IVirtualMemory* iVirtualMemory;
    IDemandPaging* iDemandPaging;

public:
    inline static const char* const ID()
    {
        return "c94a668a-7378-45b7-adbc-f6a0b32bb279";
    }

    FATComponent(MyOS::Core::IContainer& container) throw() INITSECTION;
    virtual myos_result_t initialize(Context::IContext& context,
            NVPAIR params[]) INITSECTION;
    virtual myos_result_t deinitialize(Context::IContext& context);

    virtual myos_result_t queryInterface(IComponent& requestor,
            const uuid_t& uuid, IInterface*& result);

private:
    static FATComponent* instance;
public:
    inline static FATComponent& getInstance()
    {
        return *instance;
    }

    inline static IDemandPaging& getDemandPaging()
    {
        return *getInstance().iDemandPaging;
    }

private:

    friend class IFATFSImpl; // access interfaces
    IFATFSImpl impl1;

public:
    MM::ByteAllocator allocator;

};

// within this namespace, this function can be used (GCC 3.4/4.x does not allow operator new here)
inline void* allocate(size_t s) throw(MM::OutOfMemoryException)
{
    return FATComponent::getInstance().allocator.allocate(s );
}

inline void* allocateNoThrow(size_t s) throw()
{
    return FATComponent::getInstance().allocator.allocateNoThrow(s );
}

inline void deallocate(void* p, size_t s)
{
    FATComponent::getInstance().allocator.deallocate(p, s );
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
    return MyOS::FS::FAT::allocate(size);
}

inline void* operator new[](size_t size) throw(
        MyOS::MM::OutOfMemoryException)
{
    return MyOS::FS::FAT::allocate(size);
}

inline void operator delete(void* m, size_t s)
{
    MyOS::FS::FAT::deallocate(m, s);
}

#endif	// NEW_DECLARED

#endif
