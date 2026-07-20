#ifndef FATROOTDIR_H
#define FATROOTDIR_H

#include "debug.h"
#include "types.h"

#include "Context/IDirectory.h"     // seems a bit too large for my initial purp
#include "Context/Treap.h"

#include "FATDirEntry.h"
#include "FATFileNode.h"

#include "FS/FAT/FATComponent.h"
#include "MM/new.h"

namespace MyOS { namespace FS { 
  
class IOpenFile;

namespace FAT {

using namespace Context;

class FATRootEntry;
class FAT12;

/// Implements the IDirectory interface, for now only to simply list the
/// ROOT directory on a FAT12 floppy
/**
 * Nicest would be to have memory-mapped IO also for floppies:
 * - Opening a file maps physical pages to that file, pagefaults read them in
 *   (in 8*512 blocks)
 * - Upon close() or flush(), dirty pages are written back
 */
class FATDirectory : public Context::IDirectory 
{
public:
   
   // Subdirectories are allocated dynamically
   inline void* operator new( size_t s ) {
      return MyOS::FS::FAT::allocate(s);
   }
   inline void operator delete( void* p ) {
      MyOS::FS::FAT::deallocate(p,sizeof(FATDirectory));
   }

   FATDirectory();
   FATDirectory( FATFileInfo& info, FATDirectory& parent );
   ~FATDirectory(); // called by unmount()
   inline bool isInitialized() const { return initialized; }

   void init( FATDirEntry* first, size_t _nEntries, const FAT12& fat );

   // IDirectory methods, maybe too heavy...
   virtual const myos_name_t& getDirName() const { return name; }
   virtual myos_result_t addNode( const myos_name_t& name, IInterface& i, u32 flags )
   { return E_MYOS_E_NOTIMPL; }
   virtual IInterface* removeNode( const myos_name_t& name, VERSION version )
   { return 0; }
   virtual IDirectory* createSubdirectory( const myos_name_t& name, u32 flags )
   { return 0; }
   virtual bool removeSubdirectory( IDirectory &dir ) { return false; }
   virtual myos_result_t createLink( const myos_name_t& name, IDirectory &toLink, u32 flags )
   { return E_MYOS_E_NOTIMPL; }

   // hmm, definite no-no
   virtual const buffer getIDL() const { return buffer(0,0); }
   virtual const UUID& getUUID() const { return * (UUID*) 0; }
   
   // NOTE: Should this destroy the directory physically?
   virtual bool destroy() { return false; }

   virtual myos_name_t
   findNode( const myos_name_t& fullPath, VERSION version, IInterface*& result ) const;

   // This one I will start with...
   virtual myos_result_t list( myos_name_t start, u32 recursiondepth, IO::OStream& ostream ) const;

   virtual u32 onAllNodes( IDirectoryCallback& callback, void* context, u32 flags ) const
   { return 0; }

private:
   myos_name_t name;
   FATDirectory* parent;
   
   volatile bool initialized;

private:   
   // upon start, root dir is parsed and tree is filled
   Treap< FATFileNode > files;
};
  
}}}   // namespace

#endif
