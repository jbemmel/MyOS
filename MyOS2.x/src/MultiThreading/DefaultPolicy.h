#ifndef DEFAULTPOLICY_H
#define DEFAULTPOLICY_H

#include "SchedulingPolicy.h"
#include "defs.h"
#include "MultiThreading/IMultiThreading.h"     // IRunnable
#include "debug.h"

namespace MyOS { namespace MultiThreading {

class Thread;

// Default scheduling policy
class DefaultPolicy : public SchedulingPolicy
{
      
public:
   virtual bool initialize( Context::IContext& context, Thread& init ) INITSECTION;

   /**
    * Called when a thread is created, in case the policy wants to allocate
    * some private state for it
    * 
    * Note: Called in the context of some other thread (the creator)
    */
   virtual void onThreadCreated( Thread& created );

   /**
    * Called e.g. when a thread's quantum has ended
    * 
    * @return next thread to run, can be same or other
    */
   virtual Thread& selectNext( Thread& current );

   /**
    * Called when a thread becomes ready and thus eligible for running
    * 
    * @return thread to run next, 0 to keep current
    */
   virtual void onThreadReady( Thread& ready );

   /**
    * Called when a thread starts to wait on something
    */
   virtual void onThreadWaiting( Thread& waiting );

   /**
    * Called when a thread exits (exit == current thread), in the context
    * of the exiting thread
    * 
    * @return next thread to run, cannot be equal to 'exit'
    */
   virtual Thread& onThreadExit( Thread& exit );

private:
   Thread *idleThread;   
   Thread *readylist, *readytail;   // linkedlist of ready threads   

   // Removes next thread from the ready list
   inline Thread* getNext();

   // put t on ready list, at the end
   inline void enqueue( Thread* t );

   // for debugging
   inline bool inReadylist( Thread& t );
   
   class IdleRunner : public IRunnable {
      virtual int run( NVPAIR params[] ) {
         while (1) {
            for (int i=0; i<1000000; ++i);
            CHECKPOINT('I'); 
         } 
         return 0;
      }
   } idle;   
};
   
}} // namespace

#endif
