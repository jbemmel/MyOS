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
   
	// called from IInterruptHandling   
	static void IRQ_schedule( ) ASMNAME( "IRQ_schedule" );

public:
   ThreadManager() INITSECTION;
   bool init( Context::IContext& ) INITSECTION;
   
   // called from Thread::onTimer
   //inline static Thread* endQuantum( Thread& t ) {
   //   return &(instance->policy->onEndQuantum(t));
   //}

   // called from Thread::onTimer for sleep()
   inline static void startWaiting( Thread& t ) {
      instance->policy->onThreadWaiting(t);
   }
   
   // called from Thread::onTimer
   inline static void endWaiting( Thread& t ) {
      instance->policy->onThreadReady(t);
   }

   // called from Thread::exit and onTimer
   inline static Thread& endThread( Thread& t ) {
      return (instance->policy->onThreadExit(t));
   }

   // called from Thread::onTimer
   inline static void threadReady( Thread& t ) {
      instance->policy->onThreadReady(t);
   }
};
   
}} // namespace

#endif
