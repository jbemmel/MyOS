#ifndef IInterruptHandlingImpl_H
#define IInterruptHandlingImpl_H

#include "InterruptHandling/IInterruptHandling.h"
#include "C8259.h"
#include "MM/Virtual/Selector.h"

namespace MyOS { 

namespace Context { class IContext; }
namespace MultiThreading { class Thread; }

namespace InterruptHandling {

   using Context::IContext;

/**
 * Implements interrupt handling
 * 
 * This component requires 1 function to interface with the thread scheduler:
 * IRQ_schedule
 * At the moment these are hardcoded asm names, I could use virtual set methods
 * and/or a callback interface
 */
class IInterruptHandlingImpl : public IInterruptHandling {

public:
   IInterruptHandlingImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( Context::IContext& context, NVPAIR params[] ) INITSECTION;
   void deinit( Context::IContext& context );

   virtual  myos_result_t enableIRQ( E_IRQ irq );
   virtual  myos_result_t disableIRQ( E_IRQ irq );

   virtual  myos_result_t setIRQHandler( 
      E_IRQ irq,IInterruptContext& context,u32 flags );

   virtual  myos_result_t removeIRQHandler( E_IRQ irq );

   virtual  myos_result_t setTrapHandler(
      E_INTCODES trap,myos_TRAP_f handler,u32 flags );

   virtual  myos_result_t removeTrapHandler( E_INTCODES trap );

	/**
	 * (added 17/1/05) Custom hook for APIC timer, to make sure same ASM code 
	 * is used in handling
	 */
	virtual  myos_result_t setAPICHandler( IInterruptContext& context );

   
	virtual bool triggerIRQ( E_IRQ irq );

private:
   inline static bool inIrqInternal( E_IRQ irq ) {
      return IRQhandlers[ irq - E_IRQ0 ].count != -1;
   }

   // Called from .asm after interrupt has been processed & unlocked
   // now in ThreadManager
   // static void finishINT( u32 deltaT, MultiThreading::Thread& t ) ASMNAME( "finishINT" ) REGPARM(2);

   // Trap handlers
   static void trapInvalidOpcode() ASMNAME( "trapInvalidOpcode" );
   static void trapFault() ASMNAME( "trapFault" );
   
   static void trapGPF();
   static void trapStackfault();
   static void trapSegmentfault();
   static void trapDivZero();

   // size of TRAMPOLINE code in asm
   static struct TRAMPOLINE {
      u8 assembly[8];   // align to multiples of 8!
   } const trampolines[ 16 ] ASMNAME( "IRQ_TRAMPOLINES" );

   typedef struct {
      s32 count;
      IInterruptContext* handler;
   } IRQDATA;

	// One for each IRQ + APIC timer (custom)
	static IRQDATA IRQhandlers[ 17 ] ASMNAME( "IRQDATA" );   // Used in ASM

	// Total number of handlers active, to call schedule() when reaches 0
	// static int nestCount ASMNAME( "nestCount" );		
	
	// Special segment created for interrupt handlers, identical to CS but identifiable
	MM::selector_t INTSegment;

	C8259 theC8259;   
};     
      
}} // namespaces
#endif
