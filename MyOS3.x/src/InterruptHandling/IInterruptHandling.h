#ifndef IInterruptHandling_H
#define IInterruptHandling_H

#include "Core/IInterface.h"
#include "ihtypes.h"
#include "Devices/Processor/Processor.h"

namespace MyOS
{
namespace InterruptHandling
{

inline static bool inIRQ()
{
    return Processor::CS() != 0x8;
}

/// New interface to provide a context for interrupts in one call
class IInterruptContext
{
public:
    /**
     * Callback called when a subscribed interrupt occurs
     */
    virtual void onInterrupt() = 0;
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
    
    virtual myos_result_t setAPICHandler(IInterruptContext& context) = 0;
    
    /**
     * Trigger activation of an IRQ routine from a synchronous (software)
     * context
     * 
     * This can be used to take advantage of mutual exclusion guarantees 
     * provided within the execution context of an IRQ handler
     * 
     * Note: do *not* use from within an asynchronous context running at
     * the same IRQ level!
     * 
     * @return true if succeeded, false if in same async context (dont do that!)
     */
    virtual bool triggerIRQ(E_IRQ irq) = 0;    
};

}
} // namespaces
#endif
