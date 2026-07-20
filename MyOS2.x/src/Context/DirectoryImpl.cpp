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
#include "Core/IInterface.h"
#include "Context/RepositoryComponent.h"

#include "MM/new.h"

// debug
#include "debug.h"

namespace MyOS {
namespace Context {

const char* const IDirectory::iXML = "<interface name=\"IDirectory\" uuid=\"94625155-52fc-48ad-88a8-bf40d3dfa396\"><method name=\"list\" kind=\"get\" stream=\"true\"/></interface>";

template<> inline void* TreapNode<Context::NodeIMPL>::operator new( size_t )
{
   return allocate( sizeof(TreapNode<Context::NodeIMPL>) );
}

template<> inline void TreapNode<Context::NodeIMPL>::operator delete( void* m )
{
   deallocate( m, sizeof(TreapNode<Context::NodeIMPL>) );
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

DirectoryImpl::~DirectoryImpl() throw()
{
	removeNode( SELF, VERSION::ANY() );
	if (parentDirectory) {
		removeNode( PARENT, VERSION::ANY() );
		parentDirectory->removeSubdirectory( *this );
	} else root=0;
}

myos_result_t
DirectoryImpl::addNode( const myos_name_t& name, IInterface& i, u32 flags )
{
	// CHECKPOINT( '&' );
	// DPRINTK( "\nDirectory::addNode name=%n", &name );

   // check version
   if (i.getVersion().isAny()) {
      BUG( "Invalid version argument (any) in addNode" );
      return E_MYOS_E_INVALIDARG;
   }

	const NodeIMPL newnode( name, flags & E_ALLOW_MULTIPLE_VERSIONS ? i.getVersion() : VERSION::ANY(), &i );
	myos_result_t result = E_MYOS_EEXIST;
	LOCK;
	const NodeIMPL &resultnode = subnodes.findFirst( newnode );
	if (!resultnode) {
		newnode.reuse( i, i.getVersion() );	// set version in case 'ANY' was used
		subnodes.insert( newnode );
		result = E_MYOS_SUCCESS;
      // DPRINTK( "OK" );
	} else if (flags & E_REUSE_IF_EXISTS) {
		resultnode.reuse( i, i.getVersion() );
		result = E_MYOS_REUSED;
	}
	FREE;
	return result;
}

IDirectory*
DirectoryImpl::createSubdirectory( const myos_name_t& name, u32 flags )
{
	DPRINTK( "\ncreateSubdirectory name=%n", &name );
 
	IDirectory* dir = 0;
	const NodeIMPL tofind( name, VERSION(0) );

	LOCK;
	if ( !subnodes.findFirst(tofind) ) {
		dir = new DirectoryImpl( name, this );
		if (dir) {
			subnodes.insert( NodeIMPL(name,VERSION(1,0),dir) );   // could count number of child nodes in version
		}
	} else if (flags&E_REUSE_IF_EXISTS) {
      dir = tofind.getInterface().castTo<IDirectory>();
	} // else PRINTF( "\nDirectory exists: %n", &name );
	FREE;
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
	return removeNode( dir.getDirName(), VERSION::ANY() );
	/// delete (DirectoryImpl*) &dir;	// DONT DO THIS !!
}

bool	// virtual
DirectoryImpl::destroy()
{
	delete this;
	return true;
}

IInterface*
DirectoryImpl::removeNode( const myos_name_t& _name, VERSION v )
{
   // Screen::cursor() << "\nremoveNode " << name << " this=" << (u32) this;
   NodeIMPL search( _name, v, 0 );

   LOCK;
   search = subnodes.remove( search );
   FREE;

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
      return root->findNodeRelative( myos_name_t(fullPath,1), v, result );
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

   // See if it refers to a subdirectory (-1 if not found)
   s32 sep = fullPath.indexOf( SEPARATOR );
   myos_name_t path = sep > 0 ? fullPath.substring(0, sep ) : fullPath;
   // DPRINTK( "\nFind: %n", &path );
   NodeIMPL search( path, v );

   LOCK;
   search = subnodes.findFirst( search );
   FREE;
   if (search) {
      // Something was found, though it might be overwritten below
      result = &search.getInterface();

      // if it was a subdir, continue searching (check if valid interface...)
      myos_name_t rest = fullPath.substring( sep > 0 ? sep+1 : path.length ); // past path
      // DPRINTK( "->rest=%n", &rest );      
      if (sep>=0) {
         IDirectory* dir = search.getInterface().castTo<IDirectory>();
         if (dir) {
             // ##CAST## to avoid overhead checks +virtual call of find()
             return ((DirectoryImpl*)dir)->findNodeRelative( rest, v, result );
         }             
      }
      return rest;	// return path as far as it was resolved
   }
   return fullPath;  // not resolved
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
			return dir
				? dir->list( restpath, recursiondepth, out )		// uses recursiondepth
				: result->get( restpath, 0, out );				   // try regular 'list' call
		} else return E_MYOS_E_INVALIDARG;
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

myos_result_t
DirectoryImpl::get( const char* const call, NVPAIR parameters[], IO::OStream& output ) const
{
    DPRINTK( "\nDirectoryImpl::get parameters=%x", parameters );
	return strncmp(call,"list",4)==0
		? list( myos_name_t(""), parameters && parameters[0].hasName("depth") ? parameters[0].asUnsigned() : 0, output )
		: IInterface::get( call, parameters, output );
}

u32	// virtual
DirectoryImpl::onAllNodes( IDirectoryCallback& callback, void* context, u32 flags ) const
{
		NodeIMPL::callbackParameters params = { callback, context, flags };
		return subnodes.foreach( &NodeIMPL::callback, &params );
};

}} // namespace
