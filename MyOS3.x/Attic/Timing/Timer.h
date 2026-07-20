/**
 * A new and improved design of the timing classes
 *
 * The previous version had some issues with synchronization and
 * adding / canceling timer events from synchronous contexts.
 * Plus, it got ugly
 * 
 * I will separate out the initialization classes (measure IPS, etc)
 * from this class
 * 
 * Related work: see 
 * http://www.cs.wpi.edu/~claypool/courses/502-Su03/slides/GAK+02.pdf
 */
#ifndef TIMERNEW_H
#define TIMERNEW_H

#include "PriorityHeap.h"
#include "InterruptHandling/IInterruptHandling.h"
#include "Atomic/Queue2.h"
#include "C8254.h"

namespace MyOS { namespace Timing {

using namespace InterruptHandling;

class TimerEvent;

class Timer : public IInterruptContext
{
   // IInterruptContext
   virtual MyOS::MultiThreading::Thread* onInterrupt( ISupportFunctions& helpers );

   inline void operator delete(void*) {}

   // To avoid virtual calls, TimerEvent can call Timer directly
   friend class TimerEvent;
   static Timer* instance;

public:   
   Timer() INITSECTION;
   void init() INITSECTION;
   inline virtual ~Timer() {}

   void addEvent( TimerEvent& e, u32 usAfterNow );
   void updateEvent( TimerEvent& e, u32 usAfterNow );
   void cancelEvent( TimerEvent& e, bool remove = false );
   void triggerEvent( TimerEvent& e );

   // Same as above, but to be called in onTimer() context
   void onTimer_addEvent( TimerEvent& e, u32 usAfterNow );
   void onTimer_updateEvent( TimerEvent& e, u32 usAfterNow );
   void onTimer_cancelEvent( TimerEvent& e, bool remove = false );
   void onTimer_triggerEvent( TimerEvent& e );
      
private:

   // Some parameters to tweak the size
   enum {
      E_MAXTIMERS = 256,

      // value subtracted when programming the timerchip in ticks, to
      // account for overhead (unit is processor instructions a la TSC)
      //
      // Could make this a variable, and dynamically tune it during startup...
      //
      TIMER_OVERHEAD_TICKS = 200,      
   };

   /**
    * The timer manages an eventheap (priorityheap, sorted) which is
    * only accessed from an async context (IRQ0 handler)
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

   // Matching conversion class
   C8254ConversionFactors converter;
   
   /** 
    * Timer source for this class, for now the C8254 (could use APIC timer)
    */
   C8254<E_TMR0_CNT0> timer;   
};

  
}} // namespace

#endif
