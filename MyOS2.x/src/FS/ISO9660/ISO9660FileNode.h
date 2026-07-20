#ifndef ISO9660FILENODE_H
#define ISO9660FILENODE_H

#include "Name.h"
#include "debug.h"

namespace MyOS { 
  
namespace IO { class OStream; }

namespace FS { 
 
class IOpenFile;

namespace ISO9660 {

class ISO9660FileInfo;
class FileInfo;

/// Maintains the relevant parts of ISO9660 entries in memory
class ISO9660FileNode
{
   char mszName[12]; // 8.3 format, no terminating zero
   myos_name_t name;

   // To avoid copying around, just a single reference
   ISO9660FileInfo* info;

   // prevent dynamic allocation
   inline void operator delete( void* ) {}

public:
   ISO9660FileNode( const FileInfo& e );
   ~ISO9660FileNode();

   //
   // For FAT, name comparison should be case insensitive!
   // Files are stored in capitals, so this contructors performs toupper()   
   //
   ISO9660FileNode( const myos_name_t &n ) 
      : name(mszName,0), info(0)
   { 
      // DPRINTK( "\nCopy2ucase n=%n len=%d", &n, n.length );
      name.copy2ucase(n); 
   }

   // required for treap: default constructor, operator <
   inline ISO9660FileNode() : name(mszName,0), info(0) {}
   
   ISO9660FileNode( const ISO9660FileNode& n );

   // return a copy? of the name buffer instead
   const myos_name_t& getName() const  { return name; }
   ISO9660FileInfo* getInfo() const    { return info; }
            
   /// Used after searches
   operator bool() const {
      return info != 0;  
   }

   /// Compare two nodes by name, uses strcmpa for proper number sorting
   inline bool operator <( const ISO9660FileNode& other ) const
   {
      // full match
      // s32 s = IOUtil::strncmpa( name.name, other.name.name, name.length <? other.name.length );
      
      s32 s = strncmp( name.name, other.name.name, name.length < other.name.length ? name.length : other.name.length );

      // Could order on other things too..
      return (s==0) ? name.length - other.name.length : s;
   }

   // Used in findFirst!
   inline ISO9660FileNode& operator =( const ISO9660FileNode& other )
   {
      name.copy( other.name );
      info = other.info;
      return *this;
   }

    struct toXMLparameters {
        IO::OStream& out;
        int levels;
    };
    s32 toXML( void *params );                  
};

}}}   // namespace
   
#endif
