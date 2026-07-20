#include "ITimingImpl.h"
#include "TimeUtil.h"
#include "TimerEvent.h"

#include "TimerComponent.h"

namespace MyOS
{
namespace Timer
{

int ITimerFacilityImpl::inCallback = 0;

inline void* TimerEvent::operator new( size_t )
{
    return allocate( sizeof(TimerEvent) );
}

ITimerFacilityImpl::ITimerFacilityImpl( MyOS::Core::IComponent& c )
: ITimerFacility( c, VERSION(2,0) ), timersource(&c8254)
{

}

bool
ITimerFacilityImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
    TimeUtil::initIPS( 200000000 ); // set reasonable, to prevent crashes
    c8254.init( *this );

    // Whole class should be in INITSECTION...
    class Probe : public ITimerTarget
    {
    public:
        volatile u64 endtime;

        inline Probe() : endtime(0)
        {}

        virtual void onTimer( ITimer& ) INITSECTION
        {
            endtime = TSC::read();
        }
    } probe;

    TimerEvent event( probe );    
    event.setPriority( HeapItem::HIGHEST_PRIORITY ); // set it to fire ASAP
    // event.firesAt = 0;
    eventHeap.push( &event );

    bool useAPIC = apicTimer.init( *this );
    if (useAPIC)
    {
        DPRINTK( "\nUsing local APIC as timer source" );
        // timersource = &apicTimer; do this later, if not it gets reset in probe      
    }
    else
    {
        DPRINTK( "\nUsing C8254 chip as timer source" );
    }

    Processor::allowInterrupts( true );
    c8254.doIdlePeriod(); // some initial overhead...
    u64 starttime = TSC::read(); // take snapshot of TSC just after   
    if (useAPIC) apicTimer.startCountDown(); // ..and the APIC too

    //#ifdef CONFIG_IDisplay
    Devices::Display::ScreenBuffer::getInstance().put(
            0,Devices::Display::ScreenBuffer::E_SCREENWIDTH-1,'\03',
            Devices::Display::E_RED
    );
    //#endif

    // function cannot return until probe has fired! Waste of cycles, could do
    // much better
    while (!probe.endtime) ASMVOLATILE("hlt");

    // There is some overhead before code reaches this, compensated a bit
    // by taking the snapshot *after* the timer has been started
    // Could add correction here if needed  
    u32 ticksInPeriod = (u32)(probe.endtime - starttime /* -200 */ ); // assuming <= 32bits
    if (useAPIC)
    {
        if (apicTimer.probeDone(ticksInPeriod))
        {
            timersource = &apicTimer; // from now on, use this one!
        } // else e.g. Bochs has no support for it
    }
    u32 ips = c8254.afterProbe( ticksInPeriod );

    PRINTK( "\nIPS measured at %d instructions/second", ips );
    TimeUtil::initIPS( ips );

    return true;
}

void
ITimerFacilityImpl::deinit( IContext& context )
{
    apicTimer.deinit();
    c8254.deinit();    
}

/**
 * Communication with synchronous contexts is done using a stack-allocated
 * atomic linked list of 'command' nodes
 */
class CommandNode : public /* AtomicQueue::Node */Queue::Item
{
public:
    TimerEvent& event;

    // Cannot use the priority in the event itself, since that would violate
    // heap order (for updating events that are already in the heap)
    u64 time; // aligned

    enum E_COMMAND
    {   E_ADD, E_TRIGGER, E_CANCEL, E_EXECUTED } command;

    // for E_CANCEL or E_TRIGGER
    inline CommandNode( TimerEvent& e, E_COMMAND c ) throw()
    : event(e), command(c)
    {}

    // for ADD or UPDATE
    inline CommandNode( TimerEvent& e, u64 newtime, E_COMMAND c ) throw()
    : event(e), time(newtime), command(c)
    {}
    
    inline void* operator new( size_t ) {
        return allocate( sizeof(CommandNode) );
    }
    
//    inline void operator delete( void* p ) {
//        deallocate( p, sizeof(CommandNode) );
//    }
};

// virtual, from IInterruptContext
/**
 * (usually) **ASYNC** context but also sw triggered
 * 
 * Each timer interrupt results in at most 1 event callback
 * @todo separate adding events from firing events?
 */
void
ITimerFacilityImpl::onInterrupt( ) // implements IInterruptContext::onInterrupt

{
    //bool hw = IInterruptHandling::isHardwareIRQ();
    //DPRINTK( "\nTimer::onInterrupt hw=%b", hw );

    // process any pending commands
    // DPRINTK( "\nHeapsize=%d", pending.getSize() );
    register CommandNode* c;
    while ((c = (CommandNode*) pending.dequeue()))  // note: typically only 1
    {
        TimerEvent* e = &c->event;
        u64 time = c->time;
        CommandNode::E_COMMAND command = c->command;
        // delete c;
        deallocate( c, sizeof(CommandNode) );
        switch (command)
        {
            case CommandNode::E_ADD:
            {
                e->uncancel(); // now done here
                // e->firesAt = c->time;
                // TimerEvent *top = eventHeap.top();

                // Priority is set to delta with top() event absolute time, since its only 32 bits
                eventHeap.update(e, /*top ? ( (s64) c->time - top->firesAt) : 0*/ time );
                //DPRINTK("Pushed e=%x top=%x c->time=%d top->firesAt=%d c=%b", e,
                //          eventHeap.top(), (u32) c->time, (u32) eventHeap.top()->firesAt, eventHeap.top()->isCanceled() );
                break;
            }

            case CommandNode::E_TRIGGER:
            {
                // c->command = CommandNode::E_EXECUTED; // for debugging
                int prevCallback = inCallback;
                inCallback = 1;
                bool wasTop = ( e == eventHeap.top() );
                this->triggerEvent( *e );       // this makes the callback, may enqueue a new commandNode
                inCallback = prevCallback;      // support recursive calls
                if (!wasTop && pending.isEmpty()) return;  // abort, dont re-set timer 
                break;
            }

            case CommandNode::E_CANCEL: // XX code duplication

            {
                TimerEvent *top = eventHeap.top();
                if ( e == top )
                {
                    do
                    {
                        eventHeap.pop( top );
                    } while ((top=eventHeap.top()) && top->isCanceled() );
                }
                else e->markCanceled(); // only mark it
                break;
            }

            default:
                BUG( "Bogus command" );
        }
    }

    do
    {
        TimerEvent* next = eventHeap.top();
        if (next)
        {
            // Before doing any calculations, remove any canceled events
            if ( !next->isCanceled() )
            {
                fireEvent( *next );
                break;
            }
            else
            {
                // next->uncancel();   // clear flag for next time?
                eventHeap.pop( next );
            }
        }
        else
        {
            // heap empty now, let timer do max period
            CHECKPOINT('T');

            // make heart blink
            //#ifdef CONFIG_IDisplay
            Devices::Display::ScreenBuffer::getInstance().xorput(
                    0,Devices::Display::ScreenBuffer::E_SCREENWIDTH-1,'#'
            );
            //#endif

            timersource->doIdlePeriod();
            break;
        }
    } while (1);

    // Finally, check if the thread scheduler needs a call
    // Note: IRQ0 interrupts still locked! And CS still 0x18
    //if ( inCallback==0 ) {
    //       inCallback = 1;  // still executing under IRQ0 protection
    //       MultiThreading::ThreadManager::checkReschedule();
    //       inCallback = 0;
    //}
}

// virtual 
u32
ITimerFacilityImpl::now( ETimeUnit units )
{
    return TimeUtil::now(units);
}

// virtual  
ITimer&
ITimerFacilityImpl::createTimer( ITimerTarget& callback )
{
    // DPRINTK( "\ninitTimer cb=%x", callback );

    return * new TimerEvent( callback );
    
}

void
ITimerFacilityImpl::startTimer( TimerEvent& event, u32 usAfterNow )
{
    // DPRINTK( "\n(re)startTimer(%d) CS=%x", afterUs, Processor::CS() );

    u64 abstime = TSC::read() + TimeUtil::time2ticks<us>( usAfterNow );
    if (inCallback)
    {
        // Compensate for processing overhead, to make Thread::usSleep accurate? not here
        // abstime -= 700;        // experimental
        
        event.uncancel(); // now done here, e may be in heap already!
        TimerEvent *top = eventHeap.top();
        // event.firesAt = abstime;
        eventHeap.update( &event, /*top ? (s64) abstime - top->firesAt : 0*/ abstime );

        // If new event now at top, reprogram timer!
        if ( eventHeap.top() == &event )
        {
            // DPRINTK( " ->top, reprogram" );
            fireEvent( event );
        }
    }
    else
    {
        // cannot allocate command node on the stack: no guarantee that trigger() works immediately!        
        pending.enqueue( * new CommandNode(event, abstime, CommandNode::E_ADD ) );
        timersource->trigger( );
    }
}

void
ITimerFacilityImpl::fireEvent( TimerEvent &event )
{
    s64 time_left = /* event.firesAt */ event.getPriority() - TSC::read();
    // DPRINTK( "\nFire timer time-left=%d min-period=%d @=%d", 
    //      (s32) time_left, timersource->getMinPeriodInIPS(), (u32) event.firesAt );
    /*DPRINTP( "\nTimeleft=%d #timers=%d", (s32) time_left, eventHeap.count() );
    for (int n=0; n<eventHeap.count(); ++n) {
        TimerEvent *t = eventHeap.inspect(n);
        DPRINTP( ",%d(%b)", t->getPriority(), t->isCanceled() );
    }*/
    
    
    int minPeriod = timersource->getMinPeriodInIPS();
    if (time_left < (minPeriod+ TIMER_OVERHEAD_TICKS))
    {

        // fire now, reprogram timer for next event (again, filter canceled ones)
        // DPRINTK( "\nTimer::fire time_left=%d", (s32) time_left );

        eventHeap.pop( &event );
        do
        {
            TimerEvent* after = eventHeap.top();
            if (!after)
            {
                // XX Instead of an idle period, could simply wait until next add
                timersource->doIdlePeriod();
                // fire! or maybe fire in series ??         
            }
            else if (after->isCanceled())
            {
                // after->uncancel(); when adding
                eventHeap.pop( after );
                continue;
            }
            else
            {
                // time_left += (after->firesAt - event.firesAt);
                time_left += (after->getPriority() - event.getPriority());
                
                if (time_left < minPeriod) 
                    time_left = minPeriod;
                else if (time_left > (minPeriod+TIMER_OVERHEAD_TICKS) )
                    time_left -= TIMER_OVERHEAD_TICKS;
                // ASSERTION( time_left>0, E_ERROR );
                timersource->fireAfterCPUCycles( time_left );
            }
            int prevCallback = inCallback;
            inCallback = 1;
            event.doCallback(); // fire! @todo try..catch around?
            inCallback = prevCallback;

            // Check again if there are no events, callback may have added some?
            /*if ( eventHeap.top()==0 ) */return;
        } while (1);
    } else { 
        // not time yet, reprogram timer and keep on trying
        timersource->fireAfterCPUCycles( time_left - TIMER_OVERHEAD_TICKS );
    }
}

void
ITimerFacilityImpl::triggerEvent( TimerEvent& e )
{
    // NOT_IN_IRQ; Is allowed, perhaps better not in IRQ0   
    // DPRINTK( "\ntriggerEvent inCallBack=%b CS=%x", inCallback, Processor::CS() );
    if (inCallback)
    {
        cancelEvent(e); // if e in heap, cancel it
        // e.firesAt = 0; // not strictly needed
        // DPRINTP( "\ntriggerEvent" );
        e.doCallback(); // callback may re-insert e

        //        DPRINTK( "Triggered e=%x", &e );
        //        ASSERTIONv( !e.inHeap(), E_ERROR, e.backpointer );
        //        ASSERTION( eventHeap.top() != &e, E_ERROR );        
    }
    else
    {
        // NOT_IN_IRQ;  // more strict: not in IRQ0
        
        pending.enqueue( * new CommandNode(e,CommandNode::E_TRIGGER) );
        timersource->trigger(); // method must not return until command is processed!
        // ASSERTION( c.command == CommandNode::E_EXECUTED, E_CRITICAL );
    }
}

void
ITimerFacilityImpl::cancelEvent( TimerEvent& e )
{
    //DPRINTK( "\ncancelEvent(%x) inCallBack=%b inHeap=%b top=%x count=%d", 
    //        &e, inCallback, e.inHeap(), eventHeap.top(), eventHeap.count() );
    if (inCallback)
    {
        // need to remove event if == top, continue removing while top==canceled
        TimerEvent *top = eventHeap.top();
        if ( &e == top )
        {
            do {
                eventHeap.pop( top );
            } while ((top=eventHeap.top()) && top->isCanceled() );
        }
        else e.markCanceled(); // only mark it        
    }
    else
    {        
        pending.enqueue( * new CommandNode(e,CommandNode::E_CANCEL) );
        timersource->trigger(); // method must not return until command is processed!
    }
}

}} // namespaces
