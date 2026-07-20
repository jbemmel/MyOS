#ifndef TIMEREVENT_H
#define TIMEREVENT_H

#include "timetypes.h"
#include "PriorityHeap.h"
#include "ITimer.h"

namespace MyOS { namespace Timer {
  
/**
 * Implements the ITimer interface
 * 
 * @todo add eventheap as static to this class, move code from facility here
 */  
class TimerEvent : public HeapItem, public ITimer
{
	/* ITimerTarget* */ u32 target;    // note: for 64bit, increase size!

	/**
	 * moment in time (measured in RDTSC clockticks) when this timer fires
	 */
	u64 firesAt;
	
public:
    // ITimer methods
    virtual myos_result_t start( u32 afterUs );
    virtual myos_result_t cancel();
    virtual myos_result_t trigger();
    virtual myos_result_t free();
        
	// Sets the callback and clears the cancel flag (assumes bit0 not set)
	inline void setCallback( ITimerTarget& cb ) {
		this->target = (u32) &cb;
	}

/**
 * Callback function to be implemented by classes that use this functionality
 * 
 * This function is called by the timer when the set expiration time for
 * the event passes (or has passed).
 * 
 * It is called in an *IRQ* context, so be careful about the methods you call.
 * In particular, don't call 'schedule', 'reschedule' or 'trigger'
 * as defined below, but used the 'onTimer_xx' variants instead
 */
	inline void doCallback() {
		((ITimerTarget*) (target&~1))->onTimer( *this );  // @todo try..catch?
	}
   
protected:
   // caller of new/delete and other methods here
   friend class ITimerFacilityImpl;
   
   /**
    * Canceling is implemented as setting a flag, and cleaning up canceled events from 
    * the top of the heap. This has some implications:
    * 
    * - every 'pop' needs to continue until the top item is not canceled, or the heap is empty
    * - before push, cancel flag needs to be cleared
    * - does not work when item to be canceled is at the top (need to pop then, until no more canceled)
    */ 
   inline void markCanceled()     { target |= 1; }
   inline int isCanceled() const  { return target & 1; }
   inline void uncancel()         { target &= ~1; }
   
   inline void* operator new( size_t );
   inline void operator delete( void* );

   // private Constructor, prevent subclassing
private:   
   // inline TimerEvent( ) : HeapItem(), target(0) {}     // not initializing firesAt
    TimerEvent( ITimerTarget & ) throw();
};
  
}} // namespace

#endif
