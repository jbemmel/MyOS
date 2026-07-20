#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include "types.h"
#include "DefaultPolicy.h"

namespace MyOS { namespace MultiThreading {

class Thread;

class ThreadManager
{
	static ThreadManager* instance;
	
	// (Customizable) policy for selecting the next thread to run
	SchedulingPolicy* policy;
	DefaultPolicy defaultPolicy;
   
	// called upon request at the end of interrupt processing, and directly from
	// within some Thread context
	static void reschedule( ) ASMNAME( "RESCHEDULE") /* SIGNAL_SAFE ?? */;

  // Pending reschedule request, per CPU
  // Note: GCC 4.3.0 needs the define, 4.2.3 does not...
#ifndef MAX_CPU
 #define MAX_CPU 16
#endif
  static int pendingReschedule[ MAX_CPU ] ASMNAME( "REQ_RESCHEDULE" );   // Used in ASM

public:
   ThreadManager() INITSECTION;
   bool init( Context::IContext& ) INITSECTION;

   inline static void call_reschedule() throw() {
       // !!! Need to preserve all callee-save registers!  
       // cannot do ThreadManager::reschedule();
       ASMVOLATILE( "call RESCHEDULE" : : : "%edi", "%esi", "%ebp", "%ebx", "memory" );
   }
   
   
    /**
     * Request a call to reschedule, executed after all nested interrupt executions are finished.
     * Typically called from within an interrupt context. Multiple calls during a given interrupt
     * execution stack are combined into 1 call
     *
     * @param cpu - CPU for which to request reschedule
     */
   inline static void requestReschedule( u32 cpu = 0 )
   {
      // Typically, but not necessarily in an IRQ context  
      pendingReschedule[ cpu ] = 1;  
   }

   /**
    * Used by Thread to request a rescheduling
    * 
    * Note: if immediate reschedule is needed, caller may also
    * trigger a software IRQ0 
    **/
//   static inline void requestReschedule() {
//       rescheduleRequested = 1;
//   }
   
   // Called at end of IRQ0 processing
//   static inline void checkReschedule() {
//       if (rescheduleRequested) {
//           rescheduleRequested = 0;
//           // reschedule();
//           ASMVOLATILE(
//               "lcall $0x8,$reschedule \n"   // Call scheduler, restore CS (could use ljmp)
//               "popl %eax              \n"   // remove segment from stack
//           );
//       }
//   }
   
   // called from Thread::onTimer
   //inline static Thread* endQuantum( Thread& t ) {
   //   return &(instance->policy->onEndQuantum(t));
   //}

   // called from Thread::onTimer for sleep()
//   inline static void startWaiting( Thread& t ) {
//      instance->policy->onThreadWaiting(t);
//   }
   
   // called from Thread::exit and onTimer
   inline static Thread& threadExiting( Thread& t ) {
      return instance->policy->onThreadExit(t);
   }

   // called from Thread::onTimer
   inline static void threadReady( Thread& t ) {
      instance->policy->onThreadReady(t);
   }
};
   
}} // namespace

#endif
