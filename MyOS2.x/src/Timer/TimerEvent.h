#ifndef TIMEREVENT_H
#define TIMEREVENT_H

#include "timetypes.h"
#include "PriorityHeap.h"
#include "TimerImpl.h"

namespace MyOS { namespace Timer {
  
/**
 * Designed to be allocated by users of the ITiming interface
 */  
class TimerEvent : public HeapItem  
{
public:
	// Sets the callback and clears the cancel flag
	inline void setCallback( ITimerTarget& cb ) {
		this->__unused__ = (u32) &cb;
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
		((ITimerTarget*) (__unused__&~1))->onTimer(  );
	}
   
protected:
   // caller of 'onTimer' and other methods here
   friend class TimerImpl;
   friend class ITimingImpl;
   
	void cancel()           { __unused__ |= 1; }
	int isCanceled() const  { return __unused__ & 1; }
	void uncancel()         { __unused__ &= ~1; }

   // can be called by subclasses (except while in onTimer())
   inline void schedule( u32 afterUs ) {
      TimerImpl::instance->addEvent( *this, afterUs );  
   }

   // can be called by subclasses (except while in onTimer())
   inline void reschedule( u32 afterUs ) {
      TimerImpl::instance->updateEvent( *this, afterUs );  
   }

   // can be called by subclasses (except while in onTimer())
   //inline void remove() {
   //   TimerImpl::instance->cancelEvent( *this, true );    // remove 
   //}

   // can be called by subclasses (except while in onTimer())
   inline void trigger() {
      NOT_IN_IRQ;
      TimerImpl::instance->triggerEvent( *this );  
   }

   // can be called by subclasses (*only* while in onTimer())
   inline void onTimer_schedule( u32 afterUs ) {
      TimerImpl::instance->onTimer_addEvent( *this, afterUs );  
   }
   
   inline void* operator new( size_t );
   inline void operator delete( void* );
   
   // private Constructor, prevent subclassing
private:   
   inline TimerEvent( ) : HeapItem() {}
};
  
}} // namespace

#endif
