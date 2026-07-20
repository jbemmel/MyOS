#ifndef IOREQUEST_H
#define IOREQUEST_H

#include "Atomic/Queue2.h"
#include "MM/memtypes.h"
#include "IBlockDevice.h"	// for IOCompletionHandler

namespace MyOS { namespace Devices { 

/// Request class, uses thread-based allocation (context of worker thread!)
/**
 * TODO: Currently this class is aimed only at READ requests
 * I should probably add another(?) one (or a flag bit) for WRITE requests
 */
class IORequest : public Queue::Item {

   /// The logical block number for the first block to be read
   u32 lbnStart;

   /// Number of sequential blocks in this request
   u8 seqCount;

   /// Total #blocks in the series of which this request is a part
   u8 seriesCount;

   /// Incremented for each block read, reset to 0 upon reissue
   u8 curCount;
   
   /// Similar to curCount, but not reset upon reissueContinue
   u8 dmaBlockIndex;

   IOCompletionHandler* handler;

   /// Physical DMA buffer address assigned to this series
   MM::physadr_t dmaAddress;

   // TODO: Add a 'expectedTimeMs' here, which the driver calculates when
   // the request is added and the caller can use to determine a sleeptime

public:

   // Filled in by user of this class (in .cpp file)
   inline void* operator new( size_t );
   inline void operator delete( void* );

   /**
    * @param seriesCount: if seriesCount&1!=0, this indicates that this request
    *   is the first of a series that all need to end up sequentially in some
    *   buffer.
    */
   IORequest( u32 lbs, u8 c, u8 _seriesCount, IOCompletionHandler& h )
      : lbnStart(lbs), seqCount(c), seriesCount(_seriesCount),
        curCount(0), dmaBlockIndex(0), handler(&h) {}

   /**
    * Determines log2(blockCount) equal or larger than seriesCount
    */
   inline u32 getLog2Count() const {

      // NOTE: Sometimes this function is called *after* reissue(), so
      // seqCount is new and seriesCount still has an old value
    
      for (u32 o=0, b=1; o<8; ++o, b<<=1) {
         if (b>=seriesCount) return o;
      }
      BUG( "\ngetLog2Count: unable to determine order" );
      return 32;  // XX too big
   }

   /**
    * @return E_DONE iff client is done with the request
    */
   inline u32 onRead( const buffer& b ) {
      return handler ? handler->onReadDone( *this, b ) 
                     : IOCompletionHandler::E_DONE;
   }

   inline void onError() {
      if (handler) handler->onError( lbnStart, seqCount );  
   }


   inline u32 getStartBlock() const { return lbnStart; }
   inline size_t getCount() const { return seqCount; }

   // Called by IO handler
   inline bool done() const { return curCount==seqCount; }

   // Called by handler to count down
   inline u32 nextBlock() {
      ++dmaBlockIndex; 
      return lbnStart + ++curCount; 
   }

   /**
    * In IOCompletionHandler, client can do 'return rq.reissue(...)'
    * to reissue the modified request, or E_DONE otherwise
    */
   inline u32 reissue( u32 lbs, u8 c ) {
      lbnStart = lbs;
      seqCount = c;   // Dont set seriesCount yet, needed for bufsize!
      curCount = dmaBlockIndex = 0;
      
      return IOCompletionHandler::E_REISSUE;
   }

   /**
    * ReIssues this request as the next one in the series, not resetting
    * the dmaBlockIndex index
    * 
    * This implies KEEP_BUFFER (until last block in series)
    */
   inline u32 reissueContinue( u32 lbs, u8 c ) {
      lbnStart = lbs;
      seqCount = c;
      curCount = 0;
      return IOCompletionHandler::E_REISSUE | IOCompletionHandler::E_KEEPBUFFER;      
   }
   
   // Returns the next DMA address to use
   inline u32 getDMAOffset( size_t blocksize ) const { 
      return dmaAddress + blocksize * dmaBlockIndex; 
   }
   
   inline MM::physadr_t getDMAAddress() const   { return dmaAddress; }
   inline void setDMAAddress( MM::physadr_t b ) { dmaAddress=b; }

   // @return reference to DMAaddress member
   inline MM::physadr_t& getAddr() { return dmaAddress; }
   
   /// @return true if this request is in a continued series -> keep DMA buffer
   inline bool isContinuation() const { return dmaBlockIndex > curCount; }
   
   // Called by floppythread after buffer is freed
   // (This implies it is *not* a continuation, since that needs 
   // the buffer to be sequential...
   inline void reissued() {
      seriesCount = seqCount;
      // DPRINTK( "\nREISSUED->seriesCount=%d", seriesCount );
   }
   
   /// @return Index of this request, possible as part of series
   inline u32 getPartIndex() const {
      return dmaBlockIndex - curCount;      
   }

   /**
    * Abort by caller, ie after having waited too long
    * This prevents any calls to the callback henceforth
    */
   inline void abort() {
      handler = 0;          
   }
   
};

}}   // namespace

#endif
