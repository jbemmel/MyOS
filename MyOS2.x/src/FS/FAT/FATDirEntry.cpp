#include "FATDirEntry.h"
#include "IO/OStream.h"

namespace MyOS { namespace FS { namespace FAT {

size_t 
FATDirEntry::getName( char pZname[12] ) const
{
   // Assume caller does not print deleted entries
   int i;
   // Assume space means end-of-name too
   for (i=0; i<8 && filename[i] && filename[i]!=' '; ++i) {
      pZname[i] = filename[i];
   }
   i++;  // make room for '.'
   int j;
   for (j=0; j<3 && extension[j] && extension[j]!=' '; ++j) {
      pZname[i+j] = extension[j];
   }
   if (j>0) {
      pZname[i-1] = '.';
      if (i+j<12) pZname[i+j] = 0;
   } else {
      pZname[--i] = 0;
   }      
   
   // apply fixups
   if (pZname[0]==5) pZname[0] = 0xE5;  // 0xe5 means deleted, so not stored   

   // length of name, excluding '0'
   return i+j;
}

void
FATDirEntry::print( IO::OStream& out ) const
{
   char pZname[16];
   this->getName( pZname );
   DATE date = getDateOfLastChange();
   TIME time = getTimeOfLastChange();
   out.printf( "\n<%s name=\"%s\" size=\"%d\" date=\"%d/%d/%d\" time=\"%d:%d:%d\" atts=\"%X\" first=\"%x\"/>", 
      isDirectory() ? "dir" : "file", pZname, filesize,
      date.day, date.month, date.year,
      time.hours, time.minutes, time.seconds,
      attributes, firstCluster
   );
}
  
}}}   // namespace
