#include "APICTimer.h"
#include "CPU/Processor.h"
#include "TimerComponent.h"
#include "TimeUtil.h"   // stores IPS
#include "IH/ihtypes.h"
#include "IH/LocalAPIC.h"

namespace MyOS {
namespace Timer {

using InterruptHandling::LocalAPIC;

// Multiplier for divisor, trade-off between accuracy and overflow
#define DIVISOR_N 16

void* timerHandler ASMNAME("timer_handler");

// @return false if CPU does not support it
bool APICTimer::init(InterruptHandling::IInterruptContext& timerImpl) // INITSECTION;
{
    // if (!LocalAPIC::isPresent())
        return false;  // can force usage of C8254 by always returning false

    IInterruptHandling* ih = TimerComponent::getInstance().iInterruptHandling;
    using InterruptHandling::E_APIC_TIMER_VECTOR;
    ih->setTrapHandler(E_APIC_TIMER_VECTOR, onTimer, InterruptHandling::E_NONE);

    // ih->setAPICHandler(timerImpl);
    timerHandler = &timerImpl;

    LocalAPIC::initTimer( LocalAPIC::E_DIV_1 );

    // 1 bus clocktick, corrected after probing
    this->minPeriodInIPS = 1000;

    // apic->initialCount = 1000;    // try it!
    return true;
}

/**
 * This is called "slightly" before the C8254 timer is setup to do one
 * full cycle for measurement
 */
void APICTimer::startCountDown() // INITSECTION;
{
    LocalAPIC::setTimer( 0xFFFFFFFF ); // set it at max
}

/**
 * This is called "slightly" after the IRQ fired...
 * Correction factor ??
 *
 * @return False if something wrong, e.g. 0 ticks counter
 *         This currently happens eg on Bochs without APIC support
 *        (not on real CPU)
 */
bool APICTimer::probeDone(u32 CPUticksInPeriod) // INITSECTION
{
    u32 busticks = (u32) 0xFFFFFFFF - LocalAPIC::getCurrentCount();
    if (busticks != 0)
    {
        u64 divisorN = DIVISOR_N * ((u64) CPUticksInPeriod);
        do_div( divisorN, busticks );
        if (divisorN < DIVISOR_N)
            divisorN = DIVISOR_N - 1;

        cpu2busDivisor_timesN = ((u32) divisorN) + 1; // +1 to round up
        this->minPeriodInIPS = cpu2busDivisor_timesN / DIVISOR_N; // 1 bus tick
    }
    DPRINTK("\nI counted %d busticks (CPU=%d), so minPeriod=%d", busticks,
            CPUticksInPeriod, minPeriodInIPS);

    // Test: start MP here? NO, delays dont work yet
    // this->startOtherCPU( 0x01 ); // 0xff = broadcast to all CPUs (includes self)

    return busticks != 0;
}

/***
 * **NOT** in INITSECTION!
 *
 * TODO: FIXME, APIC code currently untested!
 ***/
ASMVOLATILE (
        ".globl APIC_onTimer   			\n"
        ".align 16             			\n"
        "APIC_onTimer:         			\n"
        "pushal                         \n"
        // "incl %ss:intcount				\n"	// Keep count
        "movl %ss:apic, %eax			\n"
        "movl $0, 4*44(%eax)			\n" // send EOI to local APIC
        "movl %ss:timer_handler, %eax   \n"   // Load context in EAX (GCC calling convention)
        "movl  (%eax), %ecx             \n"   // Load method pointer at [0]
                                              // TODO load 'message' in ECX, 'cpu' in EDX
        "call  *(%ecx)                  \n"   // EBX,ESI,EDI and EBP are callee-save per C++ ABI
        "popal                          \n"
        "iretl                          \n"   // TODO call reschedule via SoftInt, like this not working
);

// virtual
void
APICTimer::fireAfterCPUCycles(u64 cpuTicks)
{
    // DPRINTK( "\nfireAfterCPUCycles: %d", cpuTicks );
    cpuTicks *= DIVISOR_N;
    do_div( cpuTicks, cpu2busDivisor_timesN );

    u32 buscycles = (u32) cpuTicks;
    if (buscycles == 0)
    {
        buscycles = 1;
    }

    // DPRINTK( "\nfireAfterCPUCycles: %d -> %d", cpuTicks, cycles );
    LocalAPIC::setTimer( buscycles ); // ensure !=0
}

// virtual
void APICTimer::doIdlePeriod()
{
    // DPRINTK( "\nAPIC::doIdlePeriod ips=%d", TimeUtil::getTicksPerSecond() );
    // this->fireAfterCPUCycles( TimeUtil::getTicksPerSecond() );   // 1 second

    LocalAPIC::setTimer( 0xFFFFFFFF );
}

}} // namespace
