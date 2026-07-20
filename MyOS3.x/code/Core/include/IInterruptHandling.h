#ifndef IInterruptHandling_H
#define IInterruptHandling_H

#include "IInterface.h"
#include "IH/ihtypes.h"
#include "CPU/Processor.h"

namespace MyOS
{
namespace InterruptHandling
{

/**
 * Note: with the wait-free synchronization enhancements, there is no longer a guarantee
 * that code is executed with a particular CS value
 */
inline static bool inIRQ()
{
    return Processor::CS() != 0x8;
}

/// New interface to provide a context for interrupts in one call
/**
 * This interface supports Message Signalled Interrupts (MSIs) as introduced with
 * PCI2.2 and PCIExpress (see http://en.wikipedia.org/wiki/Message_Signaled_Interrupts)
 *
 * Old-style line-based interrupts are emulated
 */
class IInterruptContext
{
public:
    /**
     * Callback called when a subscribed interrupt occurs
     *
     * @param message - 32-bit value representing the interrupt that was triggered,
     *                  in case of an old-style line-based interrupt this is the IRQ number (0..15)
     * @param cpu - logical identifier for the CPU executing this handler
     *
     * @throws : Nothing, implementation assumes callee catches all exceptions
     */
    virtual void onInterrupt( u32 message, cpu_id_t cpu ) throw() = 0;
};

INTERFACE( IInterruptHandling, "26468cf9-fa5b-4de7-9914-eec11e667215" )

    virtual myos_result_t enableIRQ(E_IRQ irq) = 0;

    virtual myos_result_t disableIRQ(E_IRQ irq) = 0;

    virtual myos_result_t setIRQHandler(E_IRQ irq, IInterruptContext& context,
            u32 flags) = 0;

    virtual myos_result_t removeIRQHandler(E_IRQ irq) = 0;

    virtual myos_result_t setTrapHandler(E_INTCODES trap, myos_TRAP_f handler,
            u32 flags) = 0;

    virtual myos_result_t removeTrapHandler(E_INTCODES trap) = 0;
};

}
} // namespaces
#endif
