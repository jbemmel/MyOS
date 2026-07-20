/**
 * This file (same name) is included in every Tests/ subdirectory
 * By selecting proper directories in ant.include, this will cause 
 * one of the tests to be selected at all times
 */
#include "Context/IContext.h"
#include "debug.h"
#include "Tests/TestComponent.h"

#include "TimerFacility/TimeUtil.h"
#include "MultiThreading/IThread.h"
#include "MultiProcessing/Process.h"    // XX should be IProcess

namespace MyOS { namespace Tests {

using namespace Timer;
using namespace MultiThreading;

/**
 * A test to ensure that Thread::usSleep( time ) actually sleeps for <time> us
 * 
 */
bool doTest( IContext& context, NVPAIR params[] )
{
   PRINTK( "\nStarting sleeptest: 5 seconds of quieteness..." );
   
   // This will actually not work: We are on IDLE here, and IDLE
   /* cannot sleep...
   u32 start = TimeUtil::now( us );
   IThread::do_usSleep( 5 * 1000 * 1000 );   // 5 seconds
   u32 end = TimeUtil::now( us );   
   PRINTK( "\nIDLE Sleep from %d to %d took %d us", start, end, end-start );
   */
   
   // Let's try creating a new thread to sleep then...
   static class Sleeper : public IRunnable {
      virtual int run( NVPAIR params[] ) USERSECTION {
         DPRINTP( "Sleeper executing...CS=%x", Processor::CS() );
         for (int t = 125; t < 5 * 1000 * 1000; t<<=1) {
            for (int n=0; n<1; ++n) {
                u32 start = TimeUtil::now( us );
                // DPRINTP( "\n[%d] Start sleep..", start ); 
                IThread::do_usSleep( t );   // 5 seconds
                u32 end = TimeUtil::now( us );   
                DPRINTK( "\n[%d] Side sleep(%d us) starting at %d took %d us", 
                        end, t, start, end-start );
            }
         }
         /*int t=0;
         while (++t<10) {
             IThread::do_usSleep( 1 * 1000 * 1000 );   // 1 seconds
             PRINTK( "\nOne second passed(?) now=%d (millis:%d)", 
             	TimeUtil::now( seconds ), TimeUtil::now( ms ) );
         }*/
         return 0;
      }      
   } sleeper;      

   // Two threads: works fine. 2 separate processes --> enormous differences? in optimized mode it works again
   // Perhaps writing to the screen + the lock involved blocks a thread? no, removing the lock disturbs the screen, but
   // still bad timing. Perhaps the writing itself?
   Thread& t = TestComponent::getInstance().getMT().createThread( sleeper, 0, 0/*, MM::E_DPL3*/ );
   // Thread& t2 = TestComponent::getInstance().getMT().createThread( sleeper, 0, 0/*, MM::E_DPL3*/ );
   
   // 2nd part to test: create a new process
   Process &p = TestComponent::getInstance().getMP().createProcess( sleeper, 0, 0 );
      
   // dont let sleeper get destroyed! -> now static
   // while (1);
      
   return true;
}

}}
