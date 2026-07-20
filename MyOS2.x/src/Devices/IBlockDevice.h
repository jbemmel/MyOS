#ifndef IBLOCKDEVICE_H
#define IBLOCKDEVICE_H

#include "IDevice.h"
#include "MM/memtypes.h"

namespace MyOS { 
  
class buffer;

namespace Exceptions {
   
   /// Exception thrown when no more buffers are available
   class OutOfBuffersException {
     
   };
}
  
namespace Devices {

// To be implemented by users of the BlockDriver class
class IOCompletionHandler 
{
public:
   enum E_COMPLETIONBITS {
      E_DONE         = 0,
      E_REISSUE      = 1,  //< Reissue modified request
      E_KEEPBUFFER   = 2,  //< Client keeps buffer until 'release'
   };

   /// @return E_DONE when request is done, E_REISSUE when it needs reissuing
   virtual u32 onReadDone( IORequest& rq, const buffer& data ) = 0;

   /// Client can release WriteBuffer here (or reuse it for next request)
   // virtual void onWriteDone( u32 lbnStart, size_t count, const buffer& buf ) = 0;

   virtual void onError( u32 lbnStart, size_t count ) = 0;
};

/// General interface for block devices
/**
 * This was added mainly to separate the FAT implementation from the floppy
 * driver, since in theory it could also talk to a harddisk
 * 
 * Since the aim is to make IO asynchronous, the blockdevice will work with
 * callback routines (AKA completion handlers). Here it will pass a handle
 * to a buffer where the data is available
 * 
 * Perhaps each read/write should allow passing of a Completion handler,
 * instead of once during open() ?
 * 
 */
class IBlockDevice : public IDevice
{
   u32 blocksize; // (preferred) blocksize, in bytes   

protected:
   inline IBlockDevice( Core::IComponent& c, VERSION v, u32 bs ) 
   : IDevice(c,v) { blocksize=bs; }
   
public:
   virtual myos_result_t open( ) = 0;
   
   virtual void close( ) = 0;

   /// @return This device's (preferred) blocksize, in bytes
   inline size_t getBlocksize() const { return blocksize; }

   /// Starts non-blocking read of data
   /**
    * @param seriesCount: Total #blocks of the series that this request is part of
    *                     Used to allocate a large enough contiguous buffer
    */
   virtual myos_result_t readBlocks( u32 lbnStart, u8 count, u8 seriesCount, IOCompletionHandler& h ) = 0;

   /// Release a buffer held in 'onRead' returning E_KEEPBUFFER
   virtual void releaseBuffer( buffer& buf ) = 0;

   /// Release a physical buffer held in 'onRead' returning E_KEEPBUFFER
   virtual void releasePhysicalBuffer( MM::physadr_t b, size_t blockcount ) = 0;

   /// Starts non-blocking write of data (implies that data is copied...)
   //virtual myos_result_t writeBlocks( u32 lbnStart, size_t count, const buffer& data ) = 0;
   
   /// Alternatively (more efficient since it avoids copying), the following 
   /// method could be used instead:
   //virtual buffer getWriteBuffer( size_t blockcount ) 
   //   throw (Exceptions::OutOfBuffersException) = 0;

   //virtual void releaseWriteBuffer( const buffer& buf ) = 0;
   
};

}} // namespace

#endif
