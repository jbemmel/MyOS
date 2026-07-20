#ifndef SCHEDULINGPOLICY_H
#define SCHEDULINGPOLICY_H

#include "IThread.h" // for t.state and Thread::E_SELECTED

namespace MyOS { namespace Context { class IContext; } }

namespace MyOS { namespace MultiThreading {

// Abstract base class for scheduling policies
/**
 * The scheduling policy has two important responsibilities:
 * 1. To select the thread to run at various occations (new thread created or 
 *    existing thread awoken, current thread sleep or end-of-quantum, etc)
 * 2. To determine for how long the selected thread should run
 * 
 * Currently (2) is hardcoded in ThreadManager.cpp to be 10ms (10000us)
 * 
 * TODO: True selection should only happen in onEndQuantum (rename: selectNext), 
 * 		 all other methods must return void!
 */
class SchedulingPolicy
{
public:
   virtual bool initialize( Context::IContext& context, Thread& init ) = 0;

   /**
    * Called when a thread is created, in case the policy wants to allocate
    * some private state for it
    * 
    * Note: Called in the context of some other thread (the creator)
    */
   virtual void onThreadCreated( Thread& created ) = 0;

   /**
    * Called e.g. when the current thread's quantum has ended
    * 
    * @return next thread to run (state must be 'READY')
    */
   virtual Thread& selectNext( Thread& current ) = 0;

   /**
    * Called when a thread becomes ready and thus eligible for running
    * The policy should update its internal structures
    */
   virtual void onThreadReady( Thread& ready ) = 0;

   /**
    * Called when a thread starts to wait on something
    * The policy should update its internal structures    
    */
	virtual void onThreadWaiting( Thread& waiting ) = 0;

   /**
    * TODO: COnsider semantics of this, purpose is to free exit' storage somehow
    * 
    * Called when a thread exits (exit == current thread), i.e. in the context
    * of the exiting thread
    * 
    * @return next thread to run, cannot be equal to 'exit'
    */
   virtual Thread& onThreadExit( Thread& exit ) = 0;
   
};
   
}} // namespace

#endif
