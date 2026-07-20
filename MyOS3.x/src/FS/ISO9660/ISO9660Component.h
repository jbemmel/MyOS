#ifndef ISO9660Component_H
#define ISO9660Component_H

#include "Core/IComponent.h"

#include "FS/ISO9660/IISO9660FSImpl.h"

#include "MM/Virtual/ByteAllocator.h"
#include "MM/new.h"

namespace MyOS
{
namespace FS
{
namespace ISO9660
{

/****
 * Dependencies
 ***/
using MyOS::Core::IComponent;

using MM::IVirtualMemory;
using MM::Paging::IDemandPaging;

class ISO9660Component : public IComponent
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

    ISO9660Component(MyOS::Core::IContainer& container) throw() INITSECTION;
    
    virtual myos_result_t initialize(Context::IContext& context,
            NVPAIR params[]) INITSECTION;
    virtual myos_result_t deinitialize(Context::IContext& context);

    virtual myos_result_t queryInterface(IComponent& requestor,
            const uuid_t& uuid, IInterface*& result);

private:
    static ISO9660Component* instance;
public:
    inline static ISO9660Component& getInstance()
    {
        return *instance;
    }

    static IDemandPaging& getDemandPaging()
    {
        return *getInstance().iDemandPaging;
    }

private:

    friend class IISO9660FSImpl; // access interfaces
    IISO9660FSImpl impl1;

public:
    MM::ByteAllocator allocator;

};

// within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate(size_t s) throw(MM::OutOfMemoryException)
{
    return ISO9660Component::getInstance().allocator.allocate(s );
}

inline void* allocateNoThrow(size_t s) throw()
{
    return ISO9660Component::getInstance().allocator.allocateNoThrow(s );
}

inline void deallocate(void* p, size_t s)
{
    ISO9660Component::getInstance().allocator.deallocate(p, s );
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
    return MyOS::FS::ISO9660::allocate(size);
}

inline void* operator new[](size_t size) throw(
        MyOS::MM::OutOfMemoryException)
{
    return MyOS::FS::ISO9660::allocate(size);
}

inline void operator delete(void* m, size_t s)
{
    MyOS::FS::ISO9660::deallocate(m, s);
}

#endif	// NEW_DECLARED

#endif
