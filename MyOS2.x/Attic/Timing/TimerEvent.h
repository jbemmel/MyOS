#ifndef TIMEREVENT_H
#define TIMEREVENT_H

#include "PriorityHeap.h"
#include "Timer.h"

namespace MyOS { namespace Timing {
  
/**
 * Designed to be allocated by users of the ITiming interface
 */  
class TimerEvent : public HeapItem  
{
   // use attribute in HeapItem   
#define canceled __unused__      
   // bool canceled : 1;
   
protected:
   // caller of 'onTimer' and other methods here
   friend class Timer;
   
/**
 * Callback function to be implemented by subclasses
 * 
 * This function is called by the timer when the set expiration time for
 * the event passes (or has passed).
 * 
 * It is called in an *IRQ* context, so be careful about the methods you call.
 * In particular, don't call 'schedule', 'reschedule' or 'trigger'
 * as defined below, but used the 'onTimer_xx' variants instead
 */
   virtual MyOS::MultiThreading::Thread* onTimer() = 0;   

   void cancel()           { canceled = true; }
   bool isCanceled() const { return canceled; }
   void uncancel()         { canceled = false; }

   // can be called by subclasses (except while in onTimer())
   inline void schedule( u32 afterUs ) {
      Timer::instance->addEvent( *this, afterUs );  
   }

   // can be called by subclasses (except while in onTimer())
   inline void reschedule( u32 afterUs ) {
      Timer::instance->updateEvent( *this, afterUs );  
   }

   // can be called by subclasses (except while in onTimer())
   inline void remove() {
      Timer::instance->cancelEvent( *this, true );    // remove 
   }


   // can be called by subclasses (except while in onTimer())
   inline void trigger() {
      Timer::instance->triggerEvent( *this );  
   }

   // can be called by subclasses (*only* while in onTimer())
   inline void onTimer_schedule( u32 afterUs ) {
      Timer::instance->onTimer_addEvent( *this, afterUs );  
   }


   
   // protected Constructor
   inline TimerEvent( ) : HeapItem() { canceled = false; }   
};
  
}} // namespace

#endif
