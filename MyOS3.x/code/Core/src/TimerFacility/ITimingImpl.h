#ifndef ITimingImpl_H
#define ITimingImpl_H

#include "ITiming.h"
#include "interfaces.h"
#include "signal.h"

// for TimerComponent
#include "IVirtualMemory.h"
#include "IInterruptHandling.h"

// implementation
#include "TimerEvent.h"
#include "C8254.h"
#include "APICTimer.h"
#include "PriorityHeap.h"
#include "Atomic/AtomicQueue.h"

namespace MyOS
{
namespace Timer
{

using Context::IContext;
using MM::IVirtualMemory;
using InterruptHandling::IInterruptHandling;

class ITimerFacilityImpl : public ITimerFacility, public InterruptHandling::IInterruptContext
{
public:
    ITimerFacilityImpl(MyOS::Core::IComponent& c) INITSECTION;

    bool init(IContext& context, NVPAIR params[]) INITSECTION;
    void deinit(IContext& context);

    // ITiming functions
    virtual u32 now(ETimeUnit units);
    virtual ITimer& createTimer( ITimerTarget& callback );  // @todo throw OutOfResources

    void startTimer( TimerEvent &t, u32 usAfterNow );
    void cancelEvent( TimerEvent& e, bool remove=false );
    // void triggerEvent( TimerEvent& e );

    // Thread-safe variants of critical operations
    TimerEvent* safe_startTimer( TimerEvent &t, u32 usAfterNow ) SIGNAL_SAFE;
    void safe_removeEvent( TimerEvent& e ) SIGNAL_SAFE;
    // void safe_triggerEvent( TimerEvent& e );
    TimerEvent* safe_processTimers() SIGNAL_SAFE;

private:
    // IInterruptContext
    virtual void onInterrupt( u32 message, cpu_id_t cpu ) throw() SIGNAL_SAFE;

    TimerEvent* fireEvent( TimerEvent &event ) SIGNAL_SAFE;

    void safe_processActivations() SIGNAL_SAFE;

    // Contains all possible sources, chooses one at runtime
    ITimerSource* timersource;
    C8254 c8254;
    APICTimer apicTimer;

    /**
     * The timer manages an eventheap (priorityheap, sorted) which is
     * only accessed from an async context (IRQ0 handler or APIC vector)
     */
    // Some parameters to tweak the size
    enum
    {
        E_MAXTIMERS = 256,

        // value subtracted when programming the timerchip in ticks, to
        // account for overhead (unit is processor instructions a la TSC)
        //
        // Could make this a variable, and dynamically tune it during startup...
        //
#ifdef DEBUG
        // TIMER_OVERHEAD_TICKS = 4600,     // real CPU
        // TIMER_OVERHEAD_TICKS = 740,        // Bochs
        TIMER_OVERHEAD_TICKS = 790,        // Bochs with APIC timer
#else
        // TIMER_OVERHEAD_TICKS = 1070,
        TIMER_OVERHEAD_TICKS = 720,        // Bochs
#endif

        // MIN_TIMER_TICKS = 3000, // Min. period to program, to avoid too short quota
    };
    typedef PriorityHeap<TimerEvent,E_MAXTIMERS> EVENTHEAP;
    EVENTHEAP eventHeap;

    /**
     * Lock to guarantee mutual exclusion for timer heap operations
     */
    volatile int lock;

    /**
     * FIFO queue of pending timer heap operations
     */
    AtomicQueue pending;
};

}
} // namespaces
#endif
