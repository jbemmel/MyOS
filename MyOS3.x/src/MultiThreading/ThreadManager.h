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
   
	// called at the end of per-thread timer processing
	friend class Thread;
	static void reschedule( ) ASMNAME( "reschedule");

public:
   ThreadManager() INITSECTION;
   bool init( Context::IContext& ) INITSECTION;

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
