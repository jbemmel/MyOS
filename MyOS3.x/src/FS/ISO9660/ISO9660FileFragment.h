#ifndef ISO9660FILEFRAG_H
#define ISO9660FILEFRAG_H

#include "MM/IDemandPaging.h"
#include "Devices/IBlockDevice.h"
#include "mem.h"

namespace MyOS { namespace FS { namespace ISO9660 {

using MM::physadr_t;
using MM::Exceptions::PagingException;
using Devices::IBlockDevice;

// Represents (up to) 4Kb of a file
class ISO9660FileFragment : public MM::Paging::DemandPagedEntity {

   // Need access to device for reading, any other options ?
   IBlockDevice* device;

   // Since each CD-ROM sector is 2048 bytes, each fragment is one or two secs
   // bit0 is set for a pair of sectors, bits 31..1 give the first sector
   u32 sector;
   
   // MM::Paging::DemandPagedEntity
   public: virtual physadr_t fetchNow() throw (PagingException);
   
   public:
      void* operator new[]( size_t );
      void operator delete[]( void*, size_t );
   
      // These are allocated as array
      ISO9660FileFragment() : device(0), sector(0) {}

      void setBlocks( u32 n, size_t count ) { 
         sector = (n<<1) | (count-1);
      }

      void setDevice( IBlockDevice& d ) { device = &d; }
      void unmapBuffer( physadr_t b );
      
      inline u32 getFirstBlock() const { 
         return (sector>>1);
      }
      
      inline u32 getCount() const { return 1 + (sector&1); }
};

}}}   // namespace

#endif
