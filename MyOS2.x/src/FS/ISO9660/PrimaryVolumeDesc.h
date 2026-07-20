#ifndef PRIMARYVOLDESC_H
#define PRIMARYVOLDESC_H

#include "string.h"

namespace MyOS { namespace FS { namespace ISO9660 {

struct DateTime {
   char year[4];
   char month[2]; // "01", "02", etc
   char day[2];
   char hour[2];
   char minute[2];
   char second[2];
   char hunSec[2];   
   s8 GMTOffset_15min;  // in 15-minute intervals
   
   inline bool isUnspecified() const {
      return strncmp( (const char*) this, "000000000000", 13 ) == 0;
   }
} PACKED;

/// In reality, this struct is of variable size
// The one given here is valid for the ROOT directory (34 bytes in size)
struct FileInfo {
   u8 infosize;   // size of FileInfo record, always even, eg 34 bytes
   u8 extAttrSize;
   u32 firstSector, be_0;
   size_t filesize, be_1;
   
   u8 year;    // since 1900
   u8 month;   // 1 == January
   u8 day;     // 1..31
   u8 hour;
   u8 minute;
   u8 second;
   s8 GMToffset;  // 15-min intervals
   
   enum {
      E_HIDDEN          = (1<<0),
      E_IS_DIR          = (1<<1),
      E_IS_ASSOC_FILE   = (1<<2),
      E_HAS_REC_FORMAT  = (1<<3),
      E_HAS_PERMISSIONS = (1<<4),
      E_MORE_FOLLOW     = (1<<7),
   } attributes : 8;

   u8 z0, z1;
   u16 volSequence, be_2;
   u8 idLength;
   char id[1];    // Actually variable length, padded to odd length
   
   size_t getName( char buf[] ) const {
    
      ASSERTION( idLength <= 12, E_ERROR );  // impl limit for now

      // Files have a ';' followed by version number (1), strip it
      const char* semicolon = strchr( id, ';' );
      size_t suffix = semicolon ? (semicolon-id) : idLength;
      size_t l = 12 < suffix ? 12 : suffix;
      strncpy( buf, id, l );
      return l;
   }

   size_t getSize() const { return filesize; }
   
   bool isDirectory() const { return attributes & E_IS_DIR; }
   
} PACKED;

/// Describes the Primary Volume Descriptor of a CD-ROM
/**
 * ISO9660 is a standard designed by a commission, with many endian compromises
 * This makes it very ugly...
 * 
 * see http://www.alumni.caltech.edu/~pje/iso9660.html
 */
struct PrimaryVolumeDesc 
{
   char ID[8];    // \01 CD001 \01 \00
   char systemId[32];
   char volumeId[32];
   u8 zeroes_0[8];
   u32 totalSectors, be_1;    //< Total number of sectors (also in Big Endian)
   u8 zeroes_1[32];
   u8 not_interesting[12]; 
   u32 pathTableLength, be_2; //< Length of the path table (also BE)
   u32 pathTable0StartSector;
   u32 pathTable1StartSector;
   u32 be_tableStarts[2];
   
   FileInfo root;
   
   u8 volumeSetId[128];
   u8 publisherId[128];
   u8 dataPreprId[128];
   u8 application[128];
   
   char copyrightFile[37];
   char abstractFile[37];
   char bibliographicFile[37];
   
   DateTime volumeCreationDate;
   DateTime lastModificationDate;
   DateTime expiryDate;
   DateTime startDate;

   u8 one, zero;
   u8 appReserved[512];
   u8 zeroes_2[653];   
} PACKED;
 
}}}   // namespace

#endif
