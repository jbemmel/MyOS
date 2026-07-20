#ifndef LDComponent_H
#define LDComponent_H

#include "IComponent.h"
#include "MM/ByteAllocator.h"
#include "IDynamicLoadingImpl.h"

namespace MyOS
{
namespace DynamicLoading
{

using MyOS::Core::IComponent;

class LDComponent : public IComponent
{
    static LDComponent* instance;
    IDynamicLoadingImpl impl1;

public:
    // easier
    IVirtualMemory* iVirtualMemory;
    // MM::Allocator allocator;

public:
    inline static LDComponent& getInstance() { return *instance; }
    inline static const char* const ID() { return "a09935fe-108e-49a7-bebe-dcb1d1c5d220"; }

    LDComponent(MyOS::Core::IContainer& container) throw() INITSECTION;
    virtual myos_result_t initialize(Context::IContext& context,
            NVPAIR params[]) INITSECTION;
    virtual myos_result_t deinitialize(Context::IContext& context);

    virtual myos_result_t queryInterface(IComponent& requestor,
            const uuid_t& uuid, IInterface*& result);

    inline bool loadInitialModule( Context::IContext &context ) {
        return impl1.loadInitialModule( context );
    }
};

/* within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate( size_t s ) throw (MM::OutOfMemoryException) {
   return LDComponent::getInstance().allocator.allocateAutoSize( s );
}

inline void* allocateNoThrow( size_t s ) throw () {
   return LDComponent::getInstance().allocator.allocateAutoSizeNoThrow( s );
}

inline void deallocate( void* p ) {
    LDComponent::getInstance().allocator.freeAutoSize( p );
}*/

}
} // namespaces


#endif
