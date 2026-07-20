#include "IContextImpl.h"
#include "DirectoryImpl.h"
#include "Context/RepositoryComponent.h"
#include "debug.h"

//#define CURCOMPONENT MyOS::Context::RepositoryComponent
#include "MM/new.h"

namespace MyOS { namespace Context {

IContextImpl::IContextImpl( MyOS::Core::IComponent& c )
: IContext( c, VERSION(1,0) )
{

}

bool 
IContextImpl::init( IContext& bootcontext, NVPAIR params[] ) /* INITSECTION */
{
   dir = new DirectoryImpl( myos_name_t("/"), 0 );

   // NOTE: Be aware that 'IContext' itself is added to the bootcontext, but
   // *not* to this one. Can do that here if required
   add( *this );
   
   // *DONT* link in the root directory, recursion!
   // add( *dir );

   return (bootcontext.copy( *this ) == E_MYOS_SUCCESS);
}

void 
IContextImpl::deinit( IContext& context )
{
   if (dir) {
      dir->destroy();
      dir = 0;
   }      
}


// virtual 
myos_result_t
IContextImpl::add( IInterface& intf, myos_name_t* altname /*=0 */ )  {

	// Works?
	myos_name_t name( intf.getUUID().asString(), sizeof(UUID) );
	if (altname==0) altname = &name;
	myos_result_t r = dir->addNode( *altname, intf, IDirectory::E_ALLOW_MULTIPLE_VERSIONS );
	if (r==E_MYOS_SUCCESS) {
      // also add it under its name, to be able to find it by name too
      // DPRINTK( "\nAdding by NAME: %n", &intf.getName() );
		return dir->addNode( intf.getName(), intf, IDirectory::E_ALLOW_MULTIPLE_VERSIONS );
	} else {
		DPRINTK( "#4f#\nAdding of interface %n FAILED!", altname );
		return r;
	}      
}

// virtual 
myos_result_t
IContextImpl::remove( IInterface& intf )  {
   const myos_name_t uuid = myos_name_t( intf.getUUID().asString(), sizeof(UUID) );
   // DPRINTK( "\nRemove:" );
   // DPRINTK( uuid );   
   bool r = dir->removeNode( uuid, VERSION::ANY() ) != 0;
   r &= dir->removeNode( intf.getName(), VERSION::ANY() ) != 0;
   return r ? E_MYOS_SUCCESS : E_MYOS_ENOENT;
}

// virtual 
myos_result_t
IContextImpl::lookup( const myos_name_t& id, IInterface*& result ) const {
   // originally uuid but can also be a regular name!

   if (dir->findNode( id, VERSION::ANY(), result ).length || result==0) {
      // not found
      // CHECKPOINT( '?' );
      // COUT( "\nCould not find interface " << uuid );
      // REMARK( "Interface not found" );
      DPRINTK( "#4f#\nLookup of interface %n FAILED!", &id );
      return E_MYOS_ENOENT;         
   } else return E_MYOS_SUCCESS;
}

// virtual 
myos_result_t
IContextImpl::enumerate( IO::OStream& out ) const {
   return dir->list( myos_name_t(""), 1, out );
}

// virtual  
myos_result_t 
IContextImpl::copy( IContext& to ) const
{
   // TODO
   return E_MYOS_E_NOTIMPL;
}

}}  // namespaces
