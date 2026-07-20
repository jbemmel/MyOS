#ifndef ISO9660FILEINFO_H
#define ISO9660FILEINFO_H

#include "types.h"
#include "ISO9660FileNode.h"   // getName
#include "Atomic/atomic32.h"

#include "debug.h"

namespace MyOS { 
  
namespace IO { class OStream; }
namespace Devices { class IBlockDevice; }

namespace FS { 
 
class IOpenFile;
class IOpenFileAsync;

namespace ISO9660 {

class ISO9660Directory;
class ISO9660FileFragment;

/// Maintains the relevant parts of FAT entries in memory
/**
 * Pointed to by FATFileNode (which contains the name of the file)
 */
class ISO9660FileInfo
{
   const ISO9660FileNode* node;   // backptr to node in the treap
   size_t filesize;
   ISO9660FileFragment *frags;

   // Parsed directory (if any), lazy evaluation; bit[0]==1 if DIR
   ISO9660Directory* dir;

   mutable atomic32 openfiles;
   
public:
   void* operator new( size_t );
   void operator delete( void* );
   ISO9660FileInfo( const ISO9660FileNode& node, const FileInfo& i );
   ~ISO9660FileInfo();

   // When this info is passed on to a more permanent node...
   void setNode( const ISO9660FileNode& n ) { node = &n; }
   inline bool isOwner( const ISO9660FileNode& n ) { return node==&n; }

   inline void fileClosed() const { --openfiles; }

   // Pass on certain calls
   const myos_name_t& getName() const { return node->getName(); }

   ISO9660FileFragment* getFragments() const { return frags; }
   size_t getFileSize() const { return filesize; }
         
   IOpenFile& openPaged( Devices::IBlockDevice& dev ) const;
   IOpenFileAsync& openAsync( Devices::IBlockDevice& dev ) const;
   
   // Directory support
   ISO9660Directory* loadIfDirectory( Devices::IBlockDevice& dev, ISO9660Directory& parent );
};

}}}   // namespace
   
#endif
