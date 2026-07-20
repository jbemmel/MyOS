#ifndef FATFILENODE_H
#define FATFILENODE_H

#include "Name.h"
#include "debug.h"

namespace MyOS { 
  
namespace IO { class OStream; }

namespace FS { 
 
class IOpenFile;

namespace FAT {

class FATDirEntry;
class FATFileFragment;
class FAT12;
class FATReader;
class FATFileInfo;

/// Maintains the relevant parts of FAT entries in memory
class FATFileNode
{
   char mszName[12]; // 8.3 format, no terminating zero
   myos_name_t name;

   // To avoid copying around, just a single reference
   FATFileInfo* info;

   // prevent dynamic allocation
   inline void operator delete( void* ) {}

public:
   FATFileNode( const FATDirEntry& e, const FAT12& fat );
   ~FATFileNode();

   //
   // For FAT, name comparison should be case insensitive!
   // Files are stored in capitals, so this contructors performs toupper()   
   //
   FATFileNode( const myos_name_t &n ) 
      : name(mszName,0), info(0)
   { 
      // DPRINTK( "\nCopy2ucase n=%n len=%d", &n, n.length );
      name.copy2ucase(n); 
   }

   // required for treap: default constructor, operator <
   inline FATFileNode() : name(mszName,0), info(0) {}
   
   FATFileNode( const FATFileNode& n );

   // return a copy? of the name buffer instead
   const myos_name_t& getName() const  { return name; }
   FATFileInfo* getInfo() const        { return info; }
            
   /// Used after searches
   operator bool() const {
      return info != 0;  
   }

   /// Compare two nodes by name, uses strcmpa for proper number sorting
   inline bool operator <( const FATFileNode& other ) const
   {
      // full match
      // s32 s = IOUtil::strncmpa( name.name, other.name.name, name.length <? other.name.length );
      
      s32 s = strncmp( name.name, other.name.name, min( name.length, other.name.length ) );

      // Could order on other things too..
      return (s==0) ? name.length - other.name.length : s;
   }

   // Used in findFirst!
   inline FATFileNode& operator =( const FATFileNode& other )
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
