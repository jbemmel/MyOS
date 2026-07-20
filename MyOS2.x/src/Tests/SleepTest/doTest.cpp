/**
 * This file (same name) is included in every Tests/ subdirectory
 * By selecting proper directories in ant.include, this will cause 
 * one of the tests to be selected at all times
 */
#include "Context/IContext.h"
#include "debug.h"
#include "Tests/TestComponent.h"

#include "Timer/TimeUtil.h"
#include "MultiThreading/IThread.h"

namespace MyOS { namespace Tests {

using namespace Timer;
using namespace MultiThreading;

/**
 * A test to ensure that Thread::usSleep( time ) actually sleeps for <time> us
 * 
 */
bool doTest( IContext& context, NVPAIR params[] )
{
   PRINTK( "\nStarting sleeptest: 5 seconds of quiteness..." );
   
   // This will actually not work: We are on IDLE here, and IDLE
   /* cannot sleep...
   u32 start = TimeUtil::now( us );
   IThread::do_usSleep( 5 * 1000 * 1000 );   // 5 seconds
   u32 end = TimeUtil::now( us );   
   PRINTK( "\nIDLE Sleep from %d to %d took %d us", start, end, end-start );
   */
   
   // Let's try creating a new thread to sleep then...
   class Sleeper : public IRunnable {
      virtual int run( NVPAIR params[] ) {

         for (int t = 125; t < 5 * 1000 * 1000; t<<=1) {
            u32 start = TimeUtil::now( us );
            IThread::do_usSleep( t );   // 5 seconds
            u32 end = TimeUtil::now( us );   
            PRINTK( "\nSide sleep(%d us) from %d to %d took %d us", t, start, end, end-start );       
         }
         while (1) {
             IThread::do_usSleep( 1 * 1000 * 1000 );   // 1 seconds
             PRINTK( "\nOne second passed(?) now=%d (millis:%d)", 
             	TimeUtil::now( seconds ), TimeUtil::now( ms ) );
         }
         return 0;
      }      
   } sleeper;      

   Thread& t = TestComponent::getInstance().getMT().createThread(0);

   t.start( sleeper, 0, 0 );   
   
   // dont let sleeper get destroyed!
   while (1);
      
   return true;
}

}}
