#include "FATFileFragment.h"
#include "MultiThreading/IThread.h"
#include "Devices/IORequest.h"

#include "debug.h"

namespace MyOS { namespace FS { namespace FAT {

using MultiThreading::IThread;
using MultiThreading::Thread;
using namespace Devices;

// virtual 
physadr_t 
FATFileFragment::fetchNow() throw (Exceptions::PagingException)
{
   // DPRINTK( "\nFATFileFragment::fetchNow() this=%x", this );

   // TODO: Could separate read from write handlers
   class Handler : public IOCompletionHandler {
      FATFileFragment& fragment;
      u32 cur;
      volatile physadr_t result;    // 'volatile' for busy wait, debug
      Thread& reader;
public:
   Handler( FATFileFragment& frag ) 
      : fragment(frag), cur(0), result(0), reader( Thread::currentThread() ) {}
   physadr_t getResult() const { return result; }

   // IOCompletionHandler
   virtual u32 onReadDone( IORequest& rq, const buffer& data ) {
      
      // DPRINTK( "\nonReadDone reader=%x", &reader );
      u16 next = fragment.getNextBlock( ++cur );
      if (next) { // count>0, cur<8 is checked in getNext
         return E_KEEPBUFFER | rq.reissueContinue( next & 0xFFF, next >> 12 );   
      } else {      
         result = rq.getDMAAddress();
         reader.wakeup();
         return E_KEEPBUFFER;
      }         
   }

   virtual void onError( u32 lbnStart, size_t count ) {
      reader.wakeup();
   }
     
   } handler( *this );

   // NOTE: Here, make sure the DMA buffer is page-aligned!
   // Although the last fragment may not need 8*512 bytes, dont optimize that
   // since it may result in unaligned buffers!
   device->readBlocks( 
      blocklist[0] & 0xFFF, 
      blocklist[0] >> 12, 
      8,          // don't optimize if first==last, '8' ensures page-alignment
      handler     // Make sure data>offset protected!
   );
   if (IThread::do_usSleep( 500000 )==false) {  // 500ms timeout, enough ?
      // DPRINTK( "\nonReadDone->%x", handler.getResult() );
      return handler.getResult();
   } else {
      PRINTK( "\n##ERROR: No answer within 500ms!" );
      // return 0;
      throw Exceptions::PagingException( /* timeout */ );
   }
}

// Frees the buffer kept in onRead above
void
FATFileFragment::unmapBuffer( physadr_t b )
{
   ASSERTION( device, E_ERROR );

   // Buffer is always page-aligned
   device->releasePhysicalBuffer( b, 8 );
   device = 0;
}

}}}   // namespace
