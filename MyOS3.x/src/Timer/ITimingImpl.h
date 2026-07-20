#ifndef ITimingImpl_H
#define ITimingImpl_H

#include "Timer/ITiming.h"
#include "interfaces.h"

// for TimerComponent
#include "MM/IVirtualMemory.h"
#include "InterruptHandling/IInterruptHandling.h"

// implementation
#include "TimerEvent.h"
#include "C8254.h"
#include "APICTimer.h"
#include "PriorityHeap.h"
#include "Atomic/Queue2.h"

namespace MyOS
{
namespace Timer
{

using Context::IContext;
using MM::IVirtualMemory;
using InterruptHandling::IInterruptHandling;

class ITimerFacilityImpl : public InterruptHandling::IInterruptContext, public ITimerFacility
{
public:
    ITimerFacilityImpl(MyOS::Core::IComponent& c) INITSECTION;

    bool init(IContext& context, NVPAIR params[]) INITSECTION;
    void deinit(IContext& context);

    // ITiming functions
    virtual u32 now(ETimeUnit units);
    virtual ITimer& createTimer( ITimerTarget& callback );  // @todo throw OutOfResources
    
    void startTimer( TimerEvent &t, u32 usAfterNow );
    void cancelEvent( TimerEvent& e );
    void triggerEvent( TimerEvent& e );   

private:
    // IInterruptContext
    virtual void onInterrupt( );

    // void addEvent( TimerEvent& e, u32 usAfterNow );
    // void updateEvent( TimerEvent& e, u32 usAfterNow );
    
    void fireEvent( TimerEvent &event );
    
    friend class MultiThreading::Thread;
    inline static void timerHandlerDone() {
        inCallback = 0;
    }
    
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
        TIMER_OVERHEAD_TICKS = 400,
    };
    typedef PriorityHeap<TimerEvent,E_MAXTIMERS> EVENTHEAP;
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

    /**
     * Flag indicating that a timer callback is being processed
     */
    static int inCallback;
};

}
} // namespaces
#endif
