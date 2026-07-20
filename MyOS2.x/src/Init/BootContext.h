#ifndef DUMMYCONTEXT_H
#define DUMMYCONTEXT_H

#include "Context/IContext.h"
#include "Core/IContainer.h"

namespace MyOS { namespace Init {

using Context::IContext;
using Core::IContainer;

/**
 * A context for bootstrapping, should be in INITSECTION
 * This is a simple implementation that does not use dynamic memory allocation
 */
class BootContext : public IContext {
   
   enum { MAX_INTERFACES = 16 };   
   IInterface* registered[ MAX_INTERFACES ];   
   u32 count;

   mutable IContext* realContext;  // once set, this will be used instead
   
public:
   BootContext( Core::IContainer& core );
   ~BootContext() throw();

   virtual myos_result_t get( const char* const call, MyOS::NVPAIR parameters[], IO::OStream& output ) const;
   virtual myos_result_t put( const char* const call, MyOS::NVPAIR parameters[], IO::IStream&  input );
   virtual  myos_result_t add( IInterface& intf, myos_name_t* );
   virtual  myos_result_t remove( IInterface& intf );
   virtual  myos_result_t lookup( const myos_name_t& id,IInterface*& result ) const;
   virtual  myos_result_t enumerate( IO::OStream& out ) const;
   virtual  myos_result_t copy( IContext& to ) const;
};
   
}} // namespace
   
#endif
