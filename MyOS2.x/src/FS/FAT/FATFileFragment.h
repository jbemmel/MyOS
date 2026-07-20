#ifndef FATFILEFRAG_H
#define FATFILEFRAG_H

#include "MM/IDemandPaging.h"
#include "Devices/IBlockDevice.h"
#include "mem.h"

namespace MyOS { namespace FS { namespace FAT {

using MM::physadr_t;
using Devices::IBlockDevice;

// Represents (up to) 4Kb of a file
class FATFileFragment : public MM::Paging::DemandPagedEntity {

   // Need access to device for reading, any other options ?
   IBlockDevice* device;
   
   // worst case, these are 8 loose blocks of 1 each
   // runs of up to 8 blocks are encoded in highest bits
   u16 blocklist[8];

   // MM::Paging::DemandPagedEntity
   public: virtual physadr_t fetchNow() throw (Exceptions::PagingException);
   
   public:
      void* operator new[]( size_t );
      void operator delete[]( void*, size_t );
   
      // These are allocated as array
      FATFileFragment() { memset_aligned(blocklist,0,sizeof(blocklist)); }

      /**
       * XX - Correction factor for BOOT + FAT*2 + ROOT
       * This can be calculated, for now fixed at 31
       */      
      void setBlock( u8 n, u16 v ) { blocklist[n] = v + 31; }

      void setDevice( IBlockDevice& d ) { device = &d; }
      void unmapBuffer( physadr_t b );
      
      inline u16 getNextBlock( u32 i ) const { 
         return i<8 ? blocklist[i] : 0; 
      }
};

}}}   // namespace

#endif
