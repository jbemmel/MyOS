#include "TimerImpl.h"
#include "timetypes.h"
#include "TimerEvent.h"
#include "TSC.h"
#include "TimeUtil.h"

#include "Processor/Processor.h"

#include "myosconfig.h"    // display, direct hack
#include "Devices/Display/ScreenBuffer.h"

namespace MyOS { namespace Timer {

TimerImpl* TimerImpl::instance;

TimerImpl::TimerImpl() : timersource(&c8254)  // INITSECTION
{
   instance = this;  
}

void
TimerImpl::init()  // INITSECTION
{
	TimeUtil::initIPS( 200000000 );  // set reasonable, to prevent crashes
	c8254.init( *this );

	// Whole class should be in INITSECTION...
	class Probe : public ITimerTarget
	{
	public:
		volatile u64 endtime;
		
		inline Probe() : endtime(0) {}
			
		virtual void onTimer() {
			endtime = TSC::read();	
		}
	} probe;

	TimerEvent event;
	event.setCallback( probe );		
	event.setPriority( EVENTHEAP::HIGHEST_PRIORITY ); // set it to fire ASAP
	eventHeap.push( &event );

	bool useAPIC = apicTimer.init( *this );
	if (useAPIC) {
		DPRINTK( "\nUsing local APIC as timer source" );
		// timersource = &apicTimer; do this later, if not it gets reset in probe      
	} else {
		DPRINTK( "\nUsing C8254 chip as timer source" );
	}
   
	Processor::allowInterrupts( true );
	c8254.doIdlePeriod();                    	// some initial overhead...
	u64 starttime = TSC::read();				// take snapshot of TSC just after   
	if (useAPIC) apicTimer.startCountDown(); 	// ..and the APIC too
   
#ifdef CONFIG_IDisplay
   Devices::Display::ScreenBuffer::getInstance().put(
      0,Devices::Display::ScreenBuffer::E_SCREENWIDTH-1,'\03', 
      Devices::Display::E_RED 
   );
#endif

   // function cannot return until probe has fired! Waste of cycles, could do
   // much better
	while (!probe.endtime);
	
	// There is some overhead before code reaches this, compensated a bit
	// by taking the snapshot *after* the timer has been started
	// Could add correction here if needed	
	u32 ticksInPeriod = (u32)(probe.endtime - starttime - 150);  // assuming <= 32bits
	if (useAPIC) {
		if (apicTimer.probeDone(ticksInPeriod)) {
			timersource = &apicTimer;     // from now on, use this one!
		}  // else e.g. Bochs has no support for it
	}      
	u32 ips = c8254.afterProbe( ticksInPeriod );

	DPRINTK( "\nIPS measured at %d instructions/second", ips );
	TimeUtil::initIPS( ips );
}

/**
 * Communication with synchronous contexts is done using a stack-allocated
 * atomic linked list of 'command' nodes
 */
class CommandNode : public /* AtomicQueue::Node */ Queue::Item
{
public:   
   TimerEvent& event;

   // Cannot use the priority in the event itself, since that would violate
   // heap order (for updating events that are already in the heap)
   u64 time;   // aligned

   enum  E_COMMAND { E_ADD, E_UPDATE, E_REMOVE, E_TRIGGER } command;
   
   // for REMOVE or trigger
   CommandNode( TimerEvent& e, E_COMMAND c ) 
      : event(e), command(c) {}

   // for ADD or UPDATE
   CommandNode( TimerEvent& e, u64 newtime, E_COMMAND c ) 
      : event(e), time(newtime), command(c) {}
   
};

// virtual, from IInterruptContext
// (usually) **ASYNC** context but also sw triggered
void
TimerImpl::onInterrupt( )
{
   //bool hw = IInterruptHandling::isHardwareIRQ();
   //DPRINTK( "\nTimer::onInterrupt hw=%b", hw );

   // process any pending commands
   // DPRINTK( "\nHeapsize=%d", pending.getSize() );
   register CommandNode* c;
   while ((c = (CommandNode*) pending.dequeue()))
   {
      TimerEvent* e = &c->event;
      switch (c->command) {
      case CommandNode::E_ADD:

         // Extra check in case event was canceled after 'trigger' ? unlikely...
         // if (e->isCanceled()) continue;      
         
         // could call onTimer_addEvent instead (except for recalculated time)

         if (e->inHeap()) {   // Can happen when canceled but not yet removed
            eventHeap.update( e, c->time );
         } else {           
            e->setPriority( c->time );
            eventHeap.push( e );
         }            
         break;

      case CommandNode::E_REMOVE:            
         eventHeap.pop( e );
         DPRINTK( "\nEvent %x removed with force", e );
         break;

      case CommandNode::E_TRIGGER:
         //eventHeap.pop( e );
         //return e->onTimer();    // XX Need to finish processing the queue!
         eventHeap.update( e, EVENTHEAP::HIGHEST_PRIORITY );	// was TSC::read
         break;         
      
      case CommandNode::E_UPDATE:
         // ASSERTION( e->inHeap(), E_ERROR ); normally, but not required
         // DPRINTK( "\nTimer::update ticksleft=" ); DPRINTN( c->time - TSC::read() );
         eventHeap.update( e, c->time );
         break;         
      }
   }

   // Before doing any calculations, remove any canceled events
   TimerEvent* next;
   do {
      next = (TimerEvent*) eventHeap.top();
      if (next) {
         if (next->isCanceled()) {
         	next->uncancel();	// clear flag for next time
            eventHeap.pop( next );
         }
         else break;
      } else {   // heap empty now, let timer do max period
         CHECKPOINT('T');
        
         // make heart blink
         #ifdef CONFIG_IDisplay
         Devices::Display::ScreenBuffer::getInstance().xorput(
            0,Devices::Display::ScreenBuffer::E_SCREENWIDTH-1,'#'
         );
         #endif
                
         timersource->doIdlePeriod();
         return;          
      }
   } while (1);

   // maybe abort here in case of sw ints ??
          
   s64 time_left = next->getPriority() - TSC::read() - TIMER_OVERHEAD_TICKS;
   if (time_left < (int) (timersource->getMinPeriodInIPS())) {
      
      // fire now, reprogram timer for next event (again, filter canceled ones)
      // DPRINTK( "\nTimer::fire time_left=%d", (s32) time_left );
      
      eventHeap.pop( next );
      do {
         TimerEvent* after = (TimerEvent*) eventHeap.top();
         if (!after) {
         	// XX Instead of an idle period, could simply wait until next add
            timersource->doIdlePeriod();
            next->doCallback();   // fire! or maybe fire in series ??
			return;
         } else if (after->isCanceled()) {
         	after->uncancel();
            eventHeap.pop( after );
         } else {
            time_left = after->getPriority() - TIMER_OVERHEAD_TICKS;
            timersource->fireAfterCPUCycles( time_left );
            next->doCallback();   // fire!
			return;
         }
      } while (1);
      
   } else {    // not time yet, reprogram timer and keep on trying
      timersource->fireAfterCPUCycles( time_left );
   }
}

void
TimerImpl::addEvent( TimerEvent& e, u32 usAfterNow )
{
	NOT_IN_IRQ;
   
	// PRINTK( "\naddEvent event=%x usAfterNow=%d", &e, usAfterNow );
   
	// XX careful: The event might still be in the heap! (after being canceled)
	// e.setPriority( TimeUtil::convertTime<us>( afterUs ) + TSC::read() );      

	// trigger depends on interrupts being enabled!
	//if (RARELY(Processor::interruptsEnabled()==false)) {
	//   DPRINTK( "\naddEvent with ints disabled" );
	//   onTimer_addEvent( e, usAfterNow );  // safe now
	//} else {   
	// allocate command node on the stack
	CommandNode c(e, TimeUtil::convertTime<us>( usAfterNow ) + TSC::read(), CommandNode::E_ADD );
	// e.uncancel();    // now done after removing canceled event
	pending.enqueue( c );
	timersource->trigger( );
   //}
}

void
TimerImpl::onTimer_addEvent( TimerEvent& e, u32 usAfterNow )
{
	u64 abstime = TimeUtil::convertTime<us>( usAfterNow ) + TSC::read();
	// e.uncancel();    // now done after removing canceled event
	if (e.inHeap()) {   // Can happen when canceled but not yet removed
		eventHeap.update( &e, abstime );
	} else {
		e.setPriority( abstime );
		eventHeap.push( &e );
	}
}

void 
TimerImpl::triggerEvent( TimerEvent& e )
{
	NOT_IN_IRQ;   
   
	CommandNode c(e,CommandNode::E_TRIGGER);
	pending.enqueue( c );
	timersource->trigger();
}

   
}} // namespace
