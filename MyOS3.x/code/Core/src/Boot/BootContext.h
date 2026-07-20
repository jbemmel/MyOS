#ifndef DUMMYCONTEXT_H
#define DUMMYCONTEXT_H

#include "IContext.h"
#include "IContainer.h"

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

   virtual  myos_result_t add( IInterface& intf, myos_name_t* );
   virtual  myos_result_t remove( IInterface& intf );
   virtual  IInterface& lookup( const myos_name_t& id ) const 
       throw (Context::Exceptions::InterfaceNotFoundException);
   virtual  myos_result_t enumerate( IO::OStream& out ) const;
   virtual  myos_result_t copy( IContext& to ) const;
};
   
}} // namespace
   
#endif
