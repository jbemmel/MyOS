#ifndef TIMERIMPL_H
#define TIMERIMPL_H

#include "C8254.h"
#include "APICTimer.h"

#include "PriorityHeap.h"
#include "InterruptHandling/IInterruptHandling.h"
#include "Atomic/Queue2.h"

namespace MyOS { namespace Timer {

class TimerEvent;
class timer_t;

using namespace InterruptHandling;

class TimerImpl : public IInterruptContext {
public:
	TimerImpl() INITSECTION;
	void init(  ) INITSECTION;

	inline void deinit() { c8254.deinit(); apicTimer.deinit(); }

	// IInterruptContext
	virtual void onInterrupt( );

   void addEvent( TimerEvent& e, u32 usAfterNow );
   void updateEvent( TimerEvent& e, u32 usAfterNow );
   //void cancelEvent( TimerEvent& e, bool remove = false );
   void triggerEvent( TimerEvent& e );

   // Same as above, but to be called in onTimer() context
   void onTimer_addEvent( TimerEvent& e, u32 usAfterNow );
   void onTimer_updateEvent( TimerEvent& e, u32 usAfterNow );
   //void onTimer_cancelEvent( TimerEvent& e, bool remove = false );
   void onTimer_triggerEvent( TimerEvent& e );
   
private:

   // To avoid virtual calls, TimerEvent can call TimerImpl directly
   friend class TimerEvent;
   friend class APICTimer;    // and APIC from static routine
   static TimerImpl* instance;

   // Some parameters to tweak the size
   enum {
      E_MAXTIMERS = 256,

      // value subtracted when programming the timerchip in ticks, to
      // account for overhead (unit is processor instructions a la TSC)
      //
      // Could make this a variable, and dynamically tune it during startup...
      //
      TIMER_OVERHEAD_TICKS = 400,      
   };

   // Contains all possible sources, chooses one at runtime
   ITimerSource* timersource;
   C8254 c8254;
   APICTimer apicTimer;
   
   /**
    * The timer manages an eventheap (priorityheap, sorted) which is
    * only accessed from an async context (IRQ0 handler or APIC vector)
    */
   typedef PriorityHeap<E_MAXTIMERS> EVENTHEAP;
   EVENTHEAP eventHeap;

   /**
    * Commands on the eventheap from synchronous contexts are temporarily
    * stored in an atomic linked list, and processed in the ASYNC handler
    * context
    * 
    * Due to the structure of the code (enqueue in synchronous context,
    * dequeue in protected asynchronous) it may be possible to use more
    * efficient or more simple solutions (Queue2::dequeue is complex)
    */
   // AtomicQueue pending; did not work, cannot use stack based alloc
   Queue pending;   
};
   
}} // namespace

#endif
