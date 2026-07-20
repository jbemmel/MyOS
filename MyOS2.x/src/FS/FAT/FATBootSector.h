#ifndef FATBOOTSECTOR_H
#define FATBOOTSECTOR_H

#include "debug.h"
#include "types.h"

namespace MyOS { namespace FS { namespace FAT {

/// Defines the (relevant) layout of the bootsector of a FAT12 floppy
class FATBootSector {

   // Cast this structure over a physical DMA buffer instead
   inline void* operator new( size_t );

public:
   /// @return logical sector where FAT[i] starts
   inline u32 getFATSector( u8 fatId ) const {
      ASSERTION( fatId < numberOfFATs, E_ERROR );          
      return reservedSectors + hiddenSectors + fatId*sectorsPerFAT; 
   }

   /// @return Start sector of the ROOT directory
   inline u32 getRootSector() const {
      return reservedSectors + hiddenSectors + numberOfFATs*sectorsPerFAT;      
   }

   /// @return first logical sector for a particular cluster
   inline u32 getSector( u32 cluster ) const {
      return   reservedSectors 
            +  hiddenSectors 
            +  numberOfFATs*sectorsPerFAT
            +  (rootEntries*32 + bytesPerSector - 1)/bytesPerSector
            +  (cluster-2) * sectorsPerCluster;
   }

   inline u16 getRootEntries() const { return rootEntries; }

   // debug
   void dump() const;
 
private: 
   u8 bootjmp[3];
   char vendor[8];
   u16 bytesPerSector;     // typically 512
   u8 sectorsPerCluster;   // typically 1
   u16 reservedSectors;    // including bootsector, typically 1
   u8 numberOfFATs;        // typically 2 (1 original, 1 backup)
   u16 rootEntries;        // #entries in root directory
   u16 logicalSectors;     // typically 2880 for 1.44Mb floppies
   u8 mediumDescriptor;    // e.g. 0xF0 for 1.44Mb double-density
   u16 sectorsPerFAT;      // typically 9
   u16 sectorsPerTrack;    // 18, should match mediumDescriptor
   u16 numberOfHeads;      // 2 for floppy
   u16 hiddenSectors;      // typically 0
   
   // This is where variable bits start, ignored for now
   // TODO: Volume name might be interesting to show in the Context listing

   u8 restOfBoot[ 512 - 30 ];
         
} PACKED;
  
}}}   // namespace

#endif
