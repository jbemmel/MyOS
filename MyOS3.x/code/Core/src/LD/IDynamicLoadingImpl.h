#ifndef IDynamicLoadingImpl_H
#define IDynamicLoadingImpl_H

#include "IDynamicLoading.h"

// for LDComponent.h
#include "IVirtualMemory.h"

namespace MyOS { namespace DynamicLoading {

   using Context::IContext;
   using MM::IVirtualMemory;

class IDynamicLoadingImpl : public IDynamicLoading
{
public:
    IDynamicLoadingImpl( MyOS::Core::IComponent& c ) INITSECTION;
    bool loadInitialModule( Context::IContext &context ) INITSECTION;

    virtual bool loadModule( const u8 * const elfHdr, Context::IContext &context );
};


}} // namespaces
#endif
