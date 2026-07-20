#ifndef ISO9660DIR_H
#define ISO9660DIR_H

#include "debug.h"
#include "types.h"

#include "Context/IDirectory.h"     // seems a bit too large for my initial purp
#include "Context/Treap.h"

#include "PrimaryVolumeDesc.h"
#include "ISO9660FileNode.h"

#include "FS/ISO9660/ISO9660Component.h"
#include "MM/new.h"

namespace MyOS { namespace FS { 
  
class IOpenFile;

namespace ISO9660 {

using namespace Context;

/// Represents a directory on a CD-ROM
/**
 * Main difference between ISO9660 and FAT is that ISO9660 files are _always_
 * consecutively stored on the media (required). Therefore, no FAT is needed
 */
class ISO9660Directory : public Context::IDirectory 
{
public:
   
   // Subdirectories are allocated dynamically
   inline void* operator new( size_t s ) {
      return MyOS::FS::ISO9660::allocate(s);
   }
   inline void operator delete( void* p ) {
      MyOS::FS::ISO9660::deallocate( p, sizeof(ISO9660Directory));
   }

   ISO9660Directory();
   ISO9660Directory( ISO9660FileInfo& info, ISO9660Directory& parent );
   ~ISO9660Directory(); // called by unmount()
   inline bool isInitialized() const { return initialized; }

   void init( FileInfo* first );

   // IDirectory methods, maybe too heavy...
   virtual const myos_name_t& getName() const { return name; }
   virtual myos_result_t addNode( const myos_name_t& name, IInterface& i, u32 flags )
   { return E_MYOS_ERROR; }    // XX throw ReadOnlyException instead ?
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
   ISO9660Directory* parent;
   
   volatile bool initialized;

private:   
   // upon start, root dir is parsed and tree is filled
   Treap< ISO9660FileNode > files;
};
  
}}}   // namespace

#endif
