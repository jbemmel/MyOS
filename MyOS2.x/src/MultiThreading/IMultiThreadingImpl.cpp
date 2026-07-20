#include "IMultiThreadingImpl.h"
#include "IThread.h"

#include "MM/new.h"

#include "debug.h"

namespace MyOS { namespace MultiThreading {

IMultiThreadingImpl::IMultiThreadingImpl( MyOS::Core::IComponent& c )
: IMultiThreading( c, VERSION(1,0) )
{

}

bool 
IMultiThreadingImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
   // XX passing around calls, can optimize this 
	if (!threadManager.init(context)) return false;

   /* testing...
   static class Tester : public IRunnable {
   public:
      virtual int run( NVPAIR ps[] ) {
         int r = ps[0].asNumber();
         PRINTK( "\nrun called params[0] name = %s value = %d", ps[0].getName(), r );
         // for (int c=0; c<1000000; ++c);   // delay

         // Free the param value? Yes, but don't use 'asNumber' anymore...
         // DPRINTK( "\nFreeing %x", ps[0].getValue() );
         ::operator delete[] ( (char*) ps[0].getValue(), THREAD );

         // if (( (u32)this & 0x9000 ) == 0x9000) IThread::do_exit(99);

         /// By doing 'random' sleeps stack overflow is prevented
         // (but if you create enough threads it still occurs)
         // IThread::do_usSleep( (u32)this & 0xFFFF );  // 500 ms, TEST minimum later...

         while (1) {
         // for (int i=0; i<20; ++i) {
            // actually, perhaps the return type should
            // indicate the special nature of this memory!
            // eg thread_char
            char* m1 = new (THREAD) char[1000];   // allocate memory like crazy...  
            char* m2 = new (THREAD) char[2000];
            char* m3 = new (THREAD) char[7000]; // needs 2-page alloc
            IThread::do_usSleep( 10000 );
            ::operator delete[] ( m3, THREAD );
            ::operator delete[] ( m2, THREAD );
            
            IThread::do_yield(0);
            ::operator delete[] ( m1, THREAD );
            //::delete[] m1;    // try to be alloc-friendly by using reverse order
         }

         return r;
      }     
   } tester;

   for (int t=0; t<10; ++t) {      

      // stack==1 pages, +1 guard
      // allocations in 2-page units
      Thread& thread = this->createThread(0,1); // 2 page blocks

      // This is OK! Allocate memory for the new thread, in its context
      char *num = new ( thread.allocate(2*sizeof(char)) ) char[2];
      // DPRINTK( "\nAllocated %x", num );         
      num[0] = t + '0'; num[1] = 0;
      NVPAIR ps[] = { NVPAIR("id", num), NVPAIR(0,0) };      
      this->startThread( thread, tester, ps );
   }
   // XX 'tester' and 'params' objects are destroyed when method returns
   // This can cause weird crashes...
   // while (1);
   
   // IDLE cannot Wait !
   // IThread::do_usSleep( 5000000 );  // 5000 ms = 5 sec
   // for (int i=0;i<100000000; ++i);
   
   DPRINTK( "\nKILLING stack allocated variables##@" );
*/   
   return true;        
}

void 
IMultiThreadingImpl::deinit( IContext& context )
{

}


// virtual 
myos_result_t
IMultiThreadingImpl::listThreads( IO::OStream& out ) const {
   return E_MYOS_ERROR;
}

// virtual 
Thread& 
IMultiThreadingImpl::createThread( u32 stackorder, u32 allocOrder ) 
   throw (OutOfMemoryException)
{
   return * new (stackorder) Thread(allocOrder); 
}

}}  // namespaces
