#ifdef INCLUDE_PHASE

#include "ihtypes.h"
#include "Processor/Processor.h"

namespace MyOS { namespace InterruptHandling {

inline static bool inIRQ() {
	return Processor::CS() != 0x8;
}

/// New interface to provide a context for interrupts in one call
class IInterruptContext {
public:
   /**
    * Callback called when a subscribed interrupt occurs
    */
   virtual void onInterrupt( ) = 0;
};

}}	// namespace

#else

virtual bool triggerIRQ( E_IRQ vector ) = 0;

// Detection function for IRQ handlers
inline static u32 isHardwareIRQ() {
	register int esi;
	ASM( "subl $0xFEEDFEED, %0" : "=&S"(esi) );
	return esi;
}

#endif
