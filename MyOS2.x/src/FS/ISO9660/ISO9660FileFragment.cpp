#include "ISO9660FileFragment.h"
#include "MultiThreading/IThread.h"
#include "Devices/IORequest.h"

#include "debug.h"

namespace MyOS { namespace FS { namespace ISO9660 {

using MultiThreading::IThread;
using MultiThreading::Thread;
using namespace Devices;

// virtual 
physadr_t 
ISO9660FileFragment::fetchNow() throw (Exceptions::PagingException)
{
   DPRINTK( "\nISO9660FileFragment::fetchNow() this=%x", this );

   // TODO: Could separate read from write handlers
   class Handler : public IOCompletionHandler {
      ISO9660FileFragment& fragment;
      volatile physadr_t result;    // 'volatile' for busy wait, debug
      Thread& reader;
public:
   Handler( ISO9660FileFragment& frag ) 
      : fragment(frag), result(0), reader( Thread::currentThread() ) {}

   physadr_t getResult() const { return result; }

   // IOCompletionHandler
   virtual u32 onReadDone( IORequest& rq, const buffer& data ) {
      
      DPRINTK( "\nonReadDone reader=%x count=%u d[0]=%x", 
         &reader, data.getSize(), ((u32*) data.getData())[0] );
      result = rq.getDMAAddress();
      reader.wakeup();
      return E_KEEPBUFFER;
   }

   virtual void onError( u32 lbnStart, size_t count ) {
      reader.wakeup();
   }
     
   } handler( *this );

   // NOTE: Here, make sure the DMA buffer is page-aligned!
   // Although the last fragment may not need 8*512 bytes, dont optimize that
   // since it may result in unaligned buffers!
   device->readBlocks( 
      getFirstBlock(), 
      getCount(), 
      2,          // don't optimize if !isPair, '2' ensures page-alignment
      handler     // Make sure data>offset protected!
   );
   
   // I'm assuming async IO here, this is Bochs bullshit
   // ASSERTION( handler.getResult() == 0, E_ERROR );
   if (handler.getResult()) return handler.getResult();
   
   if (IThread::do_usSleep( 500000 )==false) {  // 1000ms timeout, enough ?
      DPRINTK( "\nonReadDone->%x", handler.getResult() );
      return handler.getResult();
   } else {
      PRINTK( "\n##ERROR: No answer within 1000ms! result=%x", handler.getResult() );
      // return 0;
      throw Exceptions::PagingException( /* timeout */ );
   }
}

// Frees the buffer kept in onRead above
void
ISO9660FileFragment::unmapBuffer( physadr_t b )
{
   ASSERTION( device, E_ERROR );

   // Buffer is always page-aligned
   device->releasePhysicalBuffer( b, 8 );
   device = 0;
}

}}}   // namespace
