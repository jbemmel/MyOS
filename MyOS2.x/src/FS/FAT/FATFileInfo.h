#ifndef FATFILEINFO_H
#define FATFILEINFO_H

#include "types.h"
#include "FATFileNode.h"   // getName
#include "Atomic/atomic32.h"

#include "debug.h"

namespace MyOS { 
  
namespace IO { class OStream; }
namespace Devices { class IBlockDevice; }

namespace FS { 
 
class IOpenFile;
class IOpenFileAsync;

namespace FAT {

class FATDirEntry;
class FATDirectory;
class FATFileFragment;
class FAT12;

/// Maintains the relevant parts of FAT entries in memory
/**
 * Pointed to by FATFileNode (which contains the name of the file)
 */
class FATFileInfo
{
   const FATFileNode* node;   // backptr to node in the treap
   size_t filesize;
   FATFileFragment *frags;

   // Parsed directory (if any), lazy evaluation; bit[0]==1 if DIR
   FATDirectory* dir;

   mutable atomic32 openfiles;
   
public:
   void* operator new( size_t );
   void operator delete( void* );
   FATFileInfo( const FATFileNode& node, const FATDirEntry& e, const FAT12& fat );
   ~FATFileInfo();

   // When this info is passed on to a more permanent node...
   void setNode( const FATFileNode& n ) { node = &n; }
   inline bool isOwner( const FATFileNode& n ) { return node==&n; }

   inline void fileClosed() const { --openfiles; }

   // Pass on certain calls
   const myos_name_t& getName() const { return node->getName(); }

   FATFileFragment* getFragments() const { return frags; }
   size_t getFileSize() const { return filesize; }
         
   IOpenFile& openPaged( Devices::IBlockDevice& dev ) const;
   IOpenFileAsync& openAsync( Devices::IBlockDevice& dev ) const;
   
   // Directory support
   FATDirectory* loadIfDirectory( 
      Devices::IBlockDevice& dev, const FAT12& fat12, FATDirectory& parent );
};

}}}   // namespace
   
#endif
