#include "BootContext.h"
#include "debug.h"

namespace MyOS { namespace Init {

BootContext::BootContext( Core::IContainer& core )
   : Context::IContext( core,VERSION(1,1) ), count(0), realContext(0) 
{

}

BootContext::~BootContext() throw()
{
	// DPRINTK( "\nBootContext::~BootContext()" );
	// just in case...
	for (u32 i=0; i<count; ++i) {
		registered[i] = 0;
	}
	count=0;
}

// virtual  
myos_result_t
BootContext::add( IInterface& intf, myos_name_t* _not_used_ ) 
{
  
   // Somehow this fails to work (hangs)
   //if (realContext) {
   //   DPRINTK( "\nthis=%x realContext=%x", this, realContext );
   //   return realContext->add(intf);
   //}      
   
   if (count<MAX_INTERFACES) {
      registered[ count++ ] = &intf;
      return E_MYOS_SUCCESS;
   } else return E_MYOS_ERROR;      
}

// virtual  
myos_result_t 
BootContext::remove( IInterface& intf ) 
{ return E_MYOS_E_NOTIMPL; }

// virtual  
IInterface& 
BootContext::lookup( const myos_name_t& id ) const 
    throw (Context::Exceptions::InterfaceNotFoundException) 
{ 
    // if (realContext) return realContext->lookup(id,result);
    // DPRINTK( "\nBootContext: lookup %x(%x) len=%u", &id, id.name, id.length );
    // DPRINTK( "\nBootContext: name=%n", &id );

    //
    // Helps to detect differences in calling conventions, if this fails check the
    // compiler flags!
    //
    ASSERTION( this, E_CRITICAL );
    
    for (u32 i=0; i<this->count; ++i) {
        // Assume lookup by UUID only...
        const myos_name_t uuid( registered[i]->getUUID() );
        if ( id.equals(uuid) ) {
            // DPRINTK( "\nBootContext: found %n", &id );
            return *registered[i];
        }
    }
    DPRINTK( "\n#3f#BootContext: entry not found: %n len=%u this=%x", &id, id.length, this );
    throw Context::Exceptions::InterfaceNotFoundException( id );
}

// virtual  
myos_result_t 
BootContext::enumerate( IO::OStream& out ) const 
{ return E_MYOS_E_NOTIMPL; }                

// virtual  
myos_result_t 
BootContext::copy( IContext& to ) const
{
   ASSERTION( &to != this, E_ERROR );   
   
   myos_result_t r = E_MYOS_SUCCESS;
   for (u32 i=0; i<count && r==E_MYOS_SUCCESS; ++i) {
      r = to.add( *registered[i] );
   }
   realContext = &to;      // from now, all are added to real context instead
   return r;
}

   
}} // namespace
