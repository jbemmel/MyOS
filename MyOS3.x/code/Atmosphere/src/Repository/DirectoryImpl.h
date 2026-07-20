/***************************************************************************
                          Directoryimpl.h  -  description
                             -------------------
    begin                : Sun Sep 2 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl

TO DO:
  - perhaps rename to "context"
  - use virtual functions i.o. cast with classname
  - add remoteContext, a URL pointing to some other machine (implementation: any String?)
    Perhaps support various remote protocols: http, TFTP, LDAP, ...

 ***************************************************************************/
#ifndef DIRECTORYIMPL_H
#define DIRECTORYIMPL_H

#include "IDirectory.h"

// implementation headers
#include "Node.h"
#include "Treap.h"

#include "new.h"
#include "RepositoryComponent.h"

/// @todo Locking for Directory
#if 0
#include "SimpleLock.h"
#define DIRLOCK { SimpleLock __l; SimpleLock::PROTECTED region(__l);
#define DIRFREE }
#else
#define SPINLOCKMEMBER
#define DIRLOCK
#define DIRFREE
#endif

namespace MyOS {
namespace Context {

/**
 * Implements the IDirectory interface
 * @author Jeroen van Bemmel
 */
class DirectoryImpl : public IDirectory  {
  
   /*
   inline void* operator new( size_t s ) {
      DPRINTK( "\nDirectoryImpl::new size=%d", s );
      return MyOS::Context::allocate( s );
   }
   inline void operator delete( void* m ) {
      MyOS::Context::deallocate( m, sizeof(DirectoryImpl) );
   }*/
  
public:
	DirectoryImpl( myos_name_t name, IDirectory* parent );
	// virtual ~DirectoryImpl() throw();
	
	// IDirectory
   virtual const myos_name_t& getName() const { return dirname; }	
   virtual myos_result_t addNode( const myos_name_t& name, IInterface &i, u32 flags );
   virtual IInterface* removeNode( const myos_name_t& name, VERSION v );
   virtual IDirectory* createSubdirectory( const myos_name_t& name, u32 flags );
   virtual myos_result_t createLink( const myos_name_t& name, IDirectory &toLink, u32 flags );
   virtual bool removeSubdirectory( IDirectory &dir );
   virtual bool destroy();
   virtual myos_name_t findNode( const myos_name_t& fullPath, VERSION v, IInterface*& result ) const;

   virtual myos_result_t list( myos_name_t start, u32 recursiondepth, IO::OStream& out ) const;

   virtual u32 onAllNodes( IDirectoryCallback& callback, void* context, u32 flags ) const;

private:

   friend class IContextImpl;    // to create root dir

   // void* operator new( size_t );
   // void operator delete( void* b );

   myos_name_t findNodeRelative( const myos_name_t& relPath, VERSION v, IInterface*& result ) const;

   myos_name_t dirname;
   IDirectory* parentDirectory;
   Treap< NodeIMPL > subnodes;

   static DirectoryImpl* root;
};

}} // namespace

#endif
