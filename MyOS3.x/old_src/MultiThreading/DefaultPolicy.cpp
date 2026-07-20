#line 1 "DefaultPolicy.cpp"

// #define VERBOSE

#include "DefaultPolicy.h"
#include "Thread.h"

namespace MyOS { namespace MultiThreading {

   // Removes next thread from the ready list
   inline Thread* DefaultPolicy::getNext() {
      Thread* next = readylist;
      if (!next) {
         // CHECKPOINT('i');      
         return idleThread;
      } else {
         if ( (readylist = next->next) ) {
             next->next = 0;    // clear
         } else readytail = 0;  // queue now empty
      }
      return next;
   }   

   // put t on ready list, at the end
   inline void DefaultPolicy::enqueue( Thread* t ) {
      // ASSERTIONv( readytail != t, E_ERROR, readylist );
	  ASSERTION( t->state == Thread::E_READY, E_ERROR );
	  if (t!=idleThread) {	     
         if (readytail) { 
            readytail->next = t;
         } else readylist = t;   // => readylist also 0
         readytail = t;
      }         
   }

   // for debugging
   inline bool DefaultPolicy::inReadylist( Thread& t ) {
      for (Thread* p=readylist; p!=0; p=p->next) {
         if (p==&t) return true;
         // ASSERTION( p != p->next, E_ERROR );
         // temp
         if (p==p->next) break;
      }
      return false;
   }   

// virtual
bool 
DefaultPolicy::initialize( Context::IContext& context, Thread& init ) // INITSECTION
{
   readylist = readytail = 0;
   idleThread = &init;  // don't enqueue idle
   
/**
 * Actually, it may be better to let IDLE != INIT since then INIT can sleep
 * and wait without any problems. Initially I thought that this was waisting
 * a thread, but it seems kind of handy
 */   
//	idleThread = new ( (u32) 0 ) Thread(0);
//	idleThread->start( idle, 0, 0 );   
//
//      above crashes in Thread::select, had no time to analyse why yet
   
    return true;   
}

/**
 * Called when a thread is created, in case the policy wants to allocate
 * some private state for it
 * 
 * Note: Called in the context of some other thread (the creator)
 */
// virtual 
void 
DefaultPolicy::onThreadCreated( Thread& created )
{
   
}

/**
 * Called e.g. when a thread's quantum has ended
 * 
 * @return next thread to run, can be same or other
 */
// virtual 
Thread& 
DefaultPolicy::selectNext( Thread& current, u32 &quantumUs )
{
	ASSERTIONv( current.state != Thread::E_RUNNING, E_ERROR, current.state );
	
	// often..
#ifdef VERBOSE
	DPRINTK( "\nselectNext cur=%x cur.state=%d ready=%x", 
		&current, current.getState(), readylist );
#endif

	Thread* next = getNext();
	
	// Current is *not* part of readylist, idleThread is neither (but getNext may return idleThread)
	// Let idle have a large quantum
	if ( next == idleThread ) {
	    
	    // prefer current thread, unless it is starting to sleep
	    if ( current.state == Thread::E_READY ) {
	        if ( &current == idleThread) {
	            quantumUs = 1000 * 1000; // 1 second (@todo could allow '0' too, idle needs no timer)
	        }
	        return current;
	    }	    
	    quantumUs = 1000 * 1000; // 1 second (@todo could allow '0' too, idle needs no timer)
	} else if ( current.state == Thread::E_READY ) {
	    enqueue( &current );
	}
	return *next;
}

/**
 * Called when a thread becomes ready and thus eligible for running
 */
// virtual
void
DefaultPolicy::onThreadReady( Thread& ready )
{
#ifdef VERBOSE   
   DPRINTK( "\nonThreadReady %x cur=%x isIdle=%b", 
           &ready, &Thread::currentThread(), &Thread::currentThread() == idleThread );
#endif

   ASSERTION( !inReadylist(ready), E_ERROR );   

   // put on ready list
   enqueue( &ready );
}

/**
 * Called when a thread (current one) starts to wait on something
 * 
 *
// virtual 
void
DefaultPolicy::onThreadWaiting( Thread& waiting )
{
   // IDLE does not sleep very well...
   ASSERTION( &waiting != idleThread, E_ERROR );   
   
#ifdef VERBOSE   
   DPRINTK( "\nonThreadWaiting %x", &waiting );
#endif   
   ASSERTION( !inReadylist(waiting), E_ERROR );
}*/

/**
 * Called when a thread exits (exit == current thread), in the context
 * of the exiting thread
 * 
 */
// virtual 
Thread& 
DefaultPolicy::onThreadExit( Thread& exiting )
{
#ifdef VERBOSE   
	DPRINTK( "\nonThreadExit %x -> readylist = %x", &exiting, readylist );
#endif   
	ASSERTION( !inReadylist(exiting), E_ERROR );
	return *idleThread;
}
   
}} // namespace
