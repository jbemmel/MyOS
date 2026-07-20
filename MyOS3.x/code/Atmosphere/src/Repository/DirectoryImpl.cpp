/***************************************************************************
                          Directoryimpl.cpp  -  description
                             -------------------
    begin                : Sun Sep 2 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#include "DirectoryImpl.h"
#include "NVPAIR.h"
#include "string.h"    // strchr
#include "IInterface.h"
#include "IScriptable.h"
#include "RepositoryComponent.h"

#include "new.h"

// debug
#include "debug.h"

namespace MyOS {
namespace Context {

// const uuid_t IDirectory::ID = "94625155-52fc-48ad-88a8-bf40d3dfa396";

template<> inline void* TreapNode<Context::NodeIMPL>::operator new( size_t )
{
   return allocate( sizeof(TreapNode<Context::NodeIMPL>) );
}

template<> inline void TreapNode<Context::NodeIMPL>::operator delete( void* m )
{
   deallocate( m /*, sizeof(TreapNode<Context::NodeIMPL>)*/ );
}

#include "Treap.hpp"

DirectoryImpl* DirectoryImpl::root INITINSTANCE;

DirectoryImpl::DirectoryImpl( myos_name_t n, IDirectory *p )
    : IDirectory( RepositoryComponent::getInstance(), VERSION(1,1) )
    , dirname(n), parentDirectory(p), subnodes(NodeIMPL(myos_name_t(""),VERSION(0)))
{
    // add two default entries: . and .. (could use <this> and <parent>)
    addNode( SELF, *this, 0 );
    if (parentDirectory)
      addNode( PARENT, *parentDirectory, 0 );
    else
      root = this;
}

myos_result_t
DirectoryImpl::addNode( const myos_name_t& name, IInterface& i, u32 flags )
{
	// CHECKPOINT( '&' );
	// DPRINTK( "\nDirectory::addNode name=%n", &name );

   // check version
   if (i.getImplementationVersion().isAny()) {
      BUG( "Invalid version argument (any) in addNode" );
      return E_MYOS_E_INVALIDARG;
   }

	const NodeIMPL newnode( name, 
	        flags & E_ALLOW_MULTIPLE_VERSIONS ? i.getImplementationVersion() : VERSION::ANY(), &i );
	myos_result_t result = E_MYOS_EEXIST;
	DIRLOCK;
	const NodeIMPL &resultnode = subnodes.findFirst( newnode );
	if (!resultnode) {
		newnode.reuse( i, i.getImplementationVersion() );	// set version in case 'ANY' was used
		subnodes.insert( newnode );
		result = E_MYOS_SUCCESS;
      // DPRINTK( "OK" );
	} else if (flags & E_REUSE_IF_EXISTS) {
		resultnode.reuse( i, i.getImplementationVersion() );
		result = E_MYOS_REUSED;
	}
	DIRFREE;
	return result;
}

IDirectory*
DirectoryImpl::createSubdirectory( const myos_name_t& name, u32 flags )
{
	DPRINTK( "\ncreateSubdirectory name=%n", &name );
 
	IDirectory* dir = 0;
	const NodeIMPL tofind( name, VERSION(0) );

	DIRLOCK;
	if ( !subnodes.findFirst(tofind) ) {
		dir = new DirectoryImpl( name, this );
		if (dir) {
			subnodes.insert( NodeIMPL(name,VERSION(1,0),dir) );   // could count number of child nodes in version
		}
	} else if (flags&E_REUSE_IF_EXISTS) {
      dir = tofind.getInterface().castTo<IDirectory>();
	} // else PRINTF( "\nDirectory exists: %n", &name );
	DIRFREE;
	return dir;
}

// virtual
myos_result_t
DirectoryImpl::createLink( const myos_name_t& _name, IDirectory &toLink, u32 flags )
{
   return E_MYOS_E_NOTIMPL;
}

bool
DirectoryImpl::removeSubdirectory( IDirectory &dir )
{
	return removeNode( dir.getName(), VERSION::ANY() );
	/// delete (DirectoryImpl*) &dir;	// DONT DO THIS !!
}

bool	// virtual
DirectoryImpl::destroy()
{
    removeNode( SELF, VERSION::ANY() );
    if (parentDirectory) {
        removeNode( PARENT, VERSION::ANY() );
        parentDirectory->removeSubdirectory( *this );
    } else root=0;
        
    deallocate( this ); // allocated in IContextImpl line 24/25
	return true;
}

IInterface*
DirectoryImpl::removeNode( const myos_name_t& _name, VERSION v )
{
   // Screen::cursor() << "\nremoveNode " << name << " this=" << (u32) this;
   NodeIMPL search( _name, v, 0 );

   DIRLOCK;
   search = subnodes.remove( search );
   DIRFREE;

   // Screen::cursor() << "\nremoveNode " << name << (result ? " succeeded":" failed");
   return &search.getInterface();
}

myos_name_t
DirectoryImpl::findNode( const myos_name_t& fullPath, VERSION v, IInterface*& result ) const
{
   // DPRINTK( "\nDirectory::findNode name=%n length=%d", &fullPath, fullPath.length );
 
   if (fullPath.length==0) {
       result = 0;
       return fullPath;  // error in parameter
   } else if (fullPath.startsWith( '/' )) {
      return root->findNodeRelative( fullPath.substring(1), v, result );
   } else {
      return this->findNodeRelative( fullPath, v, result );
   }
}

myos_name_t
DirectoryImpl::findNodeRelative( const myos_name_t& fullPath, VERSION v, IInterface*& result ) const
{
   // DPRINTK( "\nDirectory::findNodeRelative name=%n length=%d isRoot=%b", 
   // &fullPath, fullPath.length, this==root );
    
   // See if it refers to self
   if (fullPath.length==0) {
      result = const_cast<DirectoryImpl*>(this);
      return fullPath;      // empty
   } 

   myos_name_t path = fullPath;   
   const DirectoryImpl *d = this;
   s32 sep = -1;
   do {   
       // See if it refers to a subdirectory (-1 if not found)
       s32 prevSep = sep;
       sep = fullPath.indexOf( SEPARATOR, sep+1 );
       if ( sep>0 ) path = fullPath.substring( prevSep+1, sep );

       if ( path.charAt(0)=='.' ) {
           char c2 = path.charAt(1);
           if ( c2==SEPARATOR ) {
               path = path.substring(2);
               continue;
           } else if (c2 == '.') {
               d = (DirectoryImpl*) d->parentDirectory;
               if ( d==0 ) return path;
               path = path.substring(2);
               continue;
           }
       }
       
       // DPRINTK( "\nFind: %n", &path );
       NodeIMPL search( path, v );

       DIRLOCK;
       search = d->subnodes.findFirst( search );
       DIRFREE;
       if (search) {
          // DPRINTK( "\nFound something" );
           
          // Something was found, though it might be overwritten below
          result = &search.getInterface();
    
          // if it was a subdir, continue searching (check if valid interface...)
          path = fullPath.substring( sep > 0 ? sep+1 : fullPath.length ); // past path
          // DPRINTK( "->rest=%n sep=%d", &path, sep );      
          if (sep>=0) {
             d = (DirectoryImpl*) search.getInterface().castTo<IDirectory>();
             if (d) {
                 continue;
             }
          }
       }
       return path;
   } while (1);
}


myos_result_t
DirectoryImpl::list( myos_name_t start, u32 recursiondepth, IO::OStream& out ) const
{
    DPRINTK( "\nDirectoryImpl::list" );
  
	// if myos_path_t != "", look further !
	if (start[0] != '\0') {
		IInterface *result=0;
		myos_name_t restpath = findNode( start, VERSION::ANY(), result);
        if (result) {
			IDirectory* dir = result->castTo <IDirectory> ();
			if (dir) {
			    // XX recursion, could convert to loop
				return dir->list( restpath, recursiondepth, out );
			} else if ( Core::IScriptable *s = result->getScriptable() ) {
				return s->get( restpath, 0, out );  // try regular 'list' call
			}
		} 
        return E_MYOS_E_INVALIDARG;
	} else {
      s32 marker = out.getMarker();
      NodeIMPL::toXMLparameters params = { out, recursiondepth };

      out.printf( "\n<DIR name=\"%n\">", &dirname );
	  subnodes.foreach( &NodeIMPL::toXML, &params );
	  out.printf( "\n</DIR>" );

	  /// out.flush(); should be done by caller
	  return (myos_result_t) (out.getMarker() - marker);
	}
}

/*
myos_result_t
DirectoryImpl::get( const char* const call, NVPAIR parameters[], IO::OStream& output ) const
{
    DPRINTK( "\nDirectoryImpl::get parameters=%x", parameters );
	return strncmp(call,"list",4)==0
		? list( myos_name_t(""), parameters && parameters[0].hasName("depth") ? parameters[0].asUnsigned() : 0, output )
		: IInterface::get( call, parameters, output );
}*/

u32	// virtual
DirectoryImpl::onAllNodes( IDirectoryCallback& callback, void* context, u32 flags ) const
{
		NodeIMPL::callbackParameters params = { callback, context, flags };
		return subnodes.foreach( &NodeIMPL::callback, &params );
};

}} // namespace
