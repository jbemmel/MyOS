#ifndef IHTYPES_H
#define IHTYPES_H

namespace MyOS { namespace InterruptHandling {

/// Mask bits to use with setInterruptHandler
enum E_HANDLERFLAGS {
   E_NONE         = 0x0,
   E_RUN_DPL0     = 0x0,   ///< Handler must be runnable at DPL0, default
   E_RUN_DPL1     = 0x1,   ///< Handler must be runnable at DPL1
   E_RUN_DPL2     = 0x2,   ///< Handler must be runnable at DPL2
   E_RUN_DPL3     = 0x3,   ///< Handler must be runnable at DPL3

   E_ENABLE_INTS  = 0x4,   ///< Routine should run with interrupts *enabled*
   E_SYNCHRONOUS  = 0x8,   ///< Installed handler is synchronous
};

// IRQ2 = cascade for 2nd IRQ controller, should not be used externally
enum E_IRQ
{
   INVALID = 0,
   E_IRQ0 = 0x68, E_IRQ1, E_IRQ2_cascade, E_IRQ3, E_IRQ4, E_IRQ5, E_IRQ6, E_IRQ7,
   E_IRQ8 = 0x70, E_IRQ9, E_IRQ10, E_IRQ11, E_IRQ12, E_IRQ13, E_IRQ14, E_IRQ15,
};

/// Function pointer to trap handler
typedef void (*myos_TRAP_f)();

enum E_INTCODES {
      E_NO_EXCEPTION          = -1,
      E_DIVIDE_BY_ZERO        = 0x0,
      E_DEBUG_TRAP            = 0x1,

      E_INVALID_OPCODE        = 0x6,
      E_DOUBLE_FAULT          = 0x8,
      E_SEGMENT_NOT_PRESENT   = 0xB,
      E_STACK_EXCEPTION       = 0xC,
      E_GPF_EXCEPTION         = 0xD,
      E_PAGE_FAULT            = 0xE,

      E_APIC_TIMER_VECTOR     = 0x31,  // programmable, but linux uses this

      /// etc.
      E_NULL_POINTER          = E_PAGE_FAULT,   // same ? different ?
};

// XX TODO: More elegant place for this
inline static void signalScheduler() {
	// normally executed in IRQ context, could assert()
	ASM( "incl softIRQ" );
}

}} // namespace

#endif
