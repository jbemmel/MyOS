#include "ITimingImpl.h"
#include "TimeUtil.h"
#include "TimerEvent.h"
#include "TimerComponent.h"
#include "new.h"

namespace MyOS {
namespace Timer {

/*
inline void* TimerEvent::operator new( size_t )
{
    return allocate( sizeof(TimerEvent) );
}*/

ITimerFacilityImpl::ITimerFacilityImpl( MyOS::Core::IComponent& c )
: ITimerFacility( c, VERSION(2,0) ), timersource(&c8254), lock(-1)
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

        virtual void onTimer( ITimer& ) throw() INITSECTION
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

    //
    // Note: AMD says "Software  which  is trying  to
    // measure  actual  processor  frequency  or  cycle-performance
    // should  use Performance  Event 76h,  CPU Clocks  not Halted"...
    //

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
    // u32 dummyCounter = 0;
    while (!probe.endtime) {
        // 'hlt' may cause TSC issues, since the CPU will enter powersave state?
        ASMVOLATILE("hlt");
        // ++dummyCounter;
    }

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
    // apicTimer.deinit();
    c8254.deinit();
}

struct AbstractActivation : public /* AtomicQueue::Node */AtomicQueue::Item
{
    virtual void execute() = 0;
};


void ITimerFacilityImpl::safe_processActivations() // SIGNAL_SAFE
{
    _UNWIND( lock );
        AbstractActivation *a = (AbstractActivation*) pending.dequeue();
        if (a) {
            a->execute();
        } else {
            _BREAK_UNWIND( lock );
        }
    _END_UNWIND( lock );
}

// virtual, from IInterruptContext
/**
 * @interrupt
 *
 * Each timer interrupt results in at most 1 event callback
 */
void
ITimerFacilityImpl::onInterrupt( u32 message, cpu_id_t cpu ) throw() // implements IInterruptContext::onInterrupt
{
    // Note: for cache optimization, it is probably better if all these structures
    // are placed together (like exception handlers: rarely used)
    struct processTimersActivation : public AbstractActivation
    {
        inline void* operator new( size_t ) throw() {
            return allocateNoThrow( sizeof(processTimersActivation) );
        }

        inline processTimersActivation() throw() {}

        virtual void execute() {
            TimerEvent *e = TimerComponent::getTFImpl().safe_processTimers();
            deallocate( this, sizeof(processTimersActivation) );

            if (e) e->doCallback(); // while holding lock ,unfortunately (but rare)
        }
    };

    TimerEvent *e = 0;
    _LOCK( lock );
        e = safe_processTimers();
    _CHECK_UNWIND( lock );
        safe_processActivations();
    _UNLOCK( lock );
        /**
         * Note: instead of allocating, could also simply increment (or set) a counter here, since
         * the interrupt does not convey any parameters. This helps to coalesce multiple interrupts,
         * and can reorder interrupts versus other operators (eg always process last or first)
         *
         * Since this is supposed to be executed rarely though, the added complexity does not
         * seem worth the effort
         */
        processTimersActivation *a = new processTimersActivation();
        if (a) pending.enqueue( *a );
    _ENDLOCK( lock );

    if (e) {
        e->doCallback();
    }
}

void
ITimerFacilityImpl::startTimer( TimerEvent &t, u32 usAfterNow )
{
    struct startTimerActivation : public AbstractActivation
    {
        TimerEvent &t;
        u32 usAfterNow;

        inline void* operator new( size_t ) throw() {
            return allocateNoThrow( sizeof(startTimerActivation) );
        }

        inline startTimerActivation( TimerEvent &_t, u32 _usAfterNow ) throw()
            : t(_t), usAfterNow(_usAfterNow) {}

        virtual void execute() {
            TimerEvent *e = TimerComponent::getTFImpl().safe_startTimer( t, usAfterNow );
            deallocate( this, sizeof(startTimerActivation) );

            if (e) e->doCallback(); // while holding lock ,unfortunately (but rare)
        }
    };

    TimerEvent *e = 0;
    _LOCK( lock );
         e = safe_startTimer( t, usAfterNow );
    _CHECK_UNWIND( lock );
        safe_processActivations();
    _UNLOCK( lock );
        startTimerActivation *a = new startTimerActivation(t,usAfterNow);
        if (a) pending.enqueue( *a );
        DPRINTK( "\nstartTimer enqueued a=%x", a );
    _ENDLOCK( lock );

    if (e) e->doCallback();     // not holding lock
}

void
ITimerFacilityImpl::cancelEvent( TimerEvent &t, bool remove )
{
    t.markCanceled();   // always mark it
    if (!remove) {      // If no removal, simply set flag and return
        return;
    }

    struct removeEventActivation : public AbstractActivation
    {
        TimerEvent &t;

        inline void* operator new( size_t ) throw() {
            return allocateNoThrow( sizeof(removeEventActivation) );
        }

        inline removeEventActivation( TimerEvent &te ) throw()
            : t(te) {}

        virtual void execute() {
            TimerComponent::getTFImpl().safe_removeEvent( t );
            deallocate( this, sizeof(removeEventActivation) );
        }
    };

    _LOCK( lock );
        safe_removeEvent( t );
    _CHECK_UNWIND( lock );
        safe_processActivations();
    _UNLOCK( lock );
        removeEventActivation *a = new removeEventActivation(t);
        if (a) pending.enqueue( *a );
    _ENDLOCK( lock );
}

/*
void
ITimerFacilityImpl::triggerEvent( TimerEvent &t )
{
    struct triggerEventActivation : public AbstractActivation
    {
        TimerEvent &t;

        inline void* operator new( size_t ) throw() {
            return allocateNoThrow( sizeof(triggerEventActivation) );
        }

        inline triggerEventActivation( TimerEvent &te ) throw() : t(te) {}

        virtual void execute() {
            TimerComponent::getTFImpl().safe_triggerEvent( t );
            deallocate( this, sizeof(triggerEventActivation) );
        }
    };

    _LOCK( lock );
        safe_triggerEvent( t );
    _CHECK_UNWIND( lock );
        safe_processActivations();
    _UNLOCK( lock );
        triggerEventActivation *a = new triggerEventActivation(t);
        if (a) pending.enqueue( *a );
    _ENDLOCK( lock );
}*/

TimerEvent*
ITimerFacilityImpl::safe_processTimers( )   // SIGNAL_SAFE
{
    do
    {
        TimerEvent* next = eventHeap.top();
        if (next)
        {
            // Before doing any calculations, remove any canceled events
            if ( !next->isCanceled() )
            {
                return fireEvent( *next );
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
            return 0;
        }
    } while (1);
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
    return * new ( allocate(sizeof(TimerEvent)) ) TimerEvent( callback );
}

TimerEvent*
ITimerFacilityImpl::safe_startTimer( TimerEvent& event, u32 usAfterNow ) // SIGNAL_SAFE
{
    // DPRINTK( "\n(re)startTimer(%d) CS=%x this=%x", usAfterNow, Processor::CS(), this );

    u64 abstime = TSC::read() + TimeUtil::time2ticks<us>( usAfterNow );

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
        return fireEvent( event );
    }
    return 0;
}

TimerEvent*
ITimerFacilityImpl::fireEvent( TimerEvent &event ) // SIGNAL_SAFE
{
    s64 time_left = /* event.firesAt */ event.getPriority() - TSC::read();
    //DPRINTK( "\nFire timer time-left=%d%d min-period=%d @=%d", (s32) (time_left>>32),
    //      (s32) time_left, timersource->getMinPeriodInIPS(), (u32) event.getPriority() );
    /*DPRINTP( "\nTimeleft=%d #timers=%d", (s32) time_left, eventHeap.count() );
    for (int n=0; n<eventHeap.count(); ++n) {
        TimerEvent *t = eventHeap.inspect(n);
        DPRINTP( ",%d(%b)", t->getPriority(), t->isCanceled() );
    }*/


    int minPeriod = timersource->getMinPeriodInIPS();
    if (time_left < (minPeriod+ TIMER_OVERHEAD_TICKS))
    {

        // fire now, reprogram timer for next event (again, filter canceled ones)
        // DPRINTK( "\nITimerFacilityImpl::fireEvent time_left=%d", (s32) time_left );

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

            // Instead of doing the callback here, do it when not holding the lock!
            // event.doCallback(); // fire! @todo try..catch around?
            return &event;
        } while (1);
    } else {
        // not time yet, reprogram timer and keep on trying
        timersource->fireAfterCPUCycles( time_left - TIMER_OVERHEAD_TICKS );
    }
    return 0;
}

/*
void
ITimerFacilityImpl::safe_triggerEvent( TimerEvent& e )
{
    e.markCanceled();  // if e in heap, cancel it
    // e.firesAt = 0;  // not strictly needed
    // DPRINTP( "\ntriggerEvent" );
    e.doCallback(); // callback may re-insert e
}
*/

void
ITimerFacilityImpl::safe_removeEvent( TimerEvent& e ) // SIGNAL_SAFE
{
    //DPRINTK( "\ncancelEvent(%x) inCallBack=%b inHeap=%b top=%x count=%d",
    //        &e, inCallback, e.inHeap(), eventHeap.top(), eventHeap.count() );

    // need to remove event if == top?
    /*TimerEvent *top = eventHeap.top();
    if ( &e == top ) {
        do {
            eventHeap.pop( top );
        } while ((top=eventHeap.top()) && top->isCanceled() );
    } else */
    e.markCanceled(); // only mark it
    if (e.inHeap()) eventHeap.pop( &e );
}

}} // namespaces
