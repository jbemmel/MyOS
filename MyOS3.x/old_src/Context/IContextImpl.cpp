#include "IContextImpl.h"
#include "DirectoryImpl.h"
#include "Context/RepositoryComponent.h"
#include "debug.h"

//#define CURCOMPONENT MyOS::Context::RepositoryComponent
#include "MM/new.h"

namespace MyOS { namespace Context {

// const uuid_t IContext::ID = "bdf79a36-5871-11d6-858d-0010a708e02e";

IContextImpl::IContextImpl( MyOS::Core::IComponent& c )
: IContext( c, VERSION(1,0) )
{

}

bool 
IContextImpl::init( IContext& bootcontext, NVPAIR params[] ) /* INITSECTION */
{
   // DPRINTK( "\nIContextImpl::init" );
   // dir = new DirectoryImpl( myos_name_t("/"), 0 );    // 22/8/07 This seems to fail, GCC new scoping has changed?   
   void *mem = Context::allocate( sizeof(DirectoryImpl) );   
   dir = new (mem) DirectoryImpl( myos_name_t("/"), 0 );
   
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

	myos_name_t name( intf.getUUID() );
	if (altname==0) altname = &name;
	myos_result_t r = dir->addNode( *altname, intf, IDirectory::E_ALLOW_MULTIPLE_VERSIONS );
	if (r==E_MYOS_SUCCESS) {
	    // also add it under its name, to be able to find it by name too      
	    // return dir->addNode( intf.getName(), intf, IDirectory::E_ALLOW_MULTIPLE_VERSIONS );
	    
	    // DPRINTK( "\nInterface added : %n", altname );
	} else {
		DPRINTK( "#4f#\nAdding of interface %n FAILED!", altname );
	}
	return r;
}

// virtual 
myos_result_t
IContextImpl::remove( IInterface& intf )  {
   const myos_name_t uuid = myos_name_t( intf.getUUID() );
   // DPRINTK( "\nRemove:" );
   // DPRINTK( uuid );   
   bool r = dir->removeNode( uuid, VERSION::ANY() ) != 0;
   // r &= dir->removeNode( intf.getName(), VERSION::ANY() ) != 0;
   return r ? E_MYOS_SUCCESS : E_MYOS_ENOENT;
}

// virtual
IInterface&
IContextImpl::lookup( const myos_name_t& id ) const throw (InterfaceNotFoundException)
{
   // originally uuid but can also be a regular name!
   IInterface *result = 0;
   if (dir->findNode( id, VERSION::ANY(), result ).length || result==0) {
      // not found
      // CHECKPOINT( '?' );
      // COUT( "\nCould not find interface " << uuid );
      // REMARK( "Interface not found" );
      DPRINTK( "#4f#\nLookup of interface %s(%x:%d) FAILED!", id.name, id.name, id.length );
      throw InterfaceNotFoundException(id);        
   } else {
       // DPRINTK( "#2f#\nLookup of interface %s(%x:%d) OK!", id.name, id.name, id.length );
       return *result;
   }
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
