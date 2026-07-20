#ifndef FATROOTENTRY_H
#define FATROOTENTRY_H

#include "debug.h"
#include "types.h"

namespace MyOS { namespace FS { namespace FAT {

/// More convenient time entry
struct TIME {
   u8 hours;
   u8 minutes;
   u8 seconds;
   u8 _notused_;
   
   inline TIME( u8 h, u8 m, u8 s ) : hours(h), minutes(m), seconds(s) {}
};

struct DATE {
   u16 year;   
   u8 month;   // range 1..12 (not 0 like in Java)
   u8 day;     // range 1..31
   
   inline DATE( u16 y, u8 m, u8 d ) : year(y), month(m), day(d) {}
};

/// Defines the layout of a root entry for a FAT12 floppy
class FATDirEntry {

   // Cast this structure over a physical DMA buffer instead
   inline void* operator new( size_t );

public:
   /// @return File is never used or was deleted
   inline bool isDeleted() const    { return ((u8)filename[0])==0xE5; }
   inline bool isLast() const       { return filename[0]==0; }
   inline bool isEmpty() const      { return isDeleted() || isLast(); }   
   inline bool isDirectory() const  { return attributes & E_DIR; }

   inline bool isSystemFile() const { return attributes & E_SYSTEM; }

   inline bool isLongFilename() const { return attributes==E_LONGNAME; }

   /**
    * MyOS extension: file denotes an alias file, i.e. a list of
    * alternative names for the file. This can be used eg for PCI
    * device drivers, where aliases are vendor+device IDs supported
    * 
    * Currently, alias files are system files with the same name as
    * some existing file and extension '.ALI' (case sensitive!)
    *
   inline bool isAliasFile() const {
      return isSystemFile() 
         && extension[0]=='A' && extension[1]=='L' && extension[2]=='I';
   }
   */

   inline bool shouldSkip() const { 
      return isDeleted() || isLongFilename()
      || (isDirectory() && filename[0]=='.');   // skip '.' and '..' now
   }

   inline u16 getFirstCluster() const { return firstCluster; }
      
   void print( IO::OStream& out ) const;

   inline TIME getTimeOfLastChange() const {
      return TIME(
         timeOfLastChange/2048,
         (timeOfLastChange%2048)/32,
         (timeOfLastChange%32)*2
      );
   }
   
   inline DATE getDateOfLastChange() const {
      return DATE(
         (dateOfLastChange/512)+1980,
         (dateOfLastChange%512)/32,
         dateOfLastChange%32
      );
   }

   size_t getName( char buf[12] ) const;
   inline size_t getSize() const { return filesize; }
   inline u8 getAttributes() const { return attributes; }

private:
   char filename[8];    // first char encodes 'erased' (s), '.', '..'
   char extension[3];
   enum {
      E_READONLY  =  (1<<0),
      E_HIDDEN    =  (1<<1),
      E_SYSTEM    =  (1<<2),
      E_LABEL     =  (1<<3),  //< Entry is not a file but a label
      E_DIR       =  (1<<4),  //< Entry is a directory
      E_ARCHIVE   =  (1<<5),
      
      //< Stupid M$ extension, skip these entries...
      E_LONGNAME  =  E_READONLY | E_HIDDEN | E_SYSTEM | E_LABEL,
   } attributes : 8;
   u8 reserved[10];
   
   u16 timeOfLastChange;   // as 2048*hours + 32*minutes + seconds/2
   u16 dateOfLastChange;   // as 512*(year-1980)+32*month+day
   u16 firstCluster;
   u32 filesize;           // in bytes
} PACKED;
  
}}}   // namespace

#endif
