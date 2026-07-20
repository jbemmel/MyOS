#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "defs.h"
#include "types.h"
//#include "MM/Virtual/Selector.h"

/* enum ESpeedUnit { MHz, GHz };	// for ClockSpeed */

namespace MyOS {

/**
 * Type to identify a logical/physical CPU core
 */
typedef u32 cpu_id_t;

/// Represents a processor with registers
/**
 * This singleton class is responsible for dealing with all differences between
 * CPU vendors (most notibaly Intel and AMD)
 *
 * All MSR / CPUID queries and settings should go through here, I dont want any
 * checks spread throughout the system
 */
class Processor
{
public:
   /// Flags register bits, @see EFLAGS
   typedef u32 flags_t;
   typedef u16 selector_t;

   /// Flagbits in the EFLAGS register
	enum EFLAGS
	{
		FLAG_CARRY 		= (1 << 0),
		FLAG_PARITY 	= (1 << 2),
		FLAG_AUXILARY   = (1 << 4),
		FLAG_ZERO 		= (1 << 6),
		FLAG_SIGN 		= (1 << 7),
		FLAG_TRAP 		= (1 << 8),
		FLAG_INT 		= (1 << 9),
		FLAG_DIR		= (1 << 10),
		FLAG_OVERFLOW	= (1 << 11),

        /// IO privilege level 0..3
		FLAG_IOPL0	= 0,
		FLAG_IOPL1	= (1 << 12),
		FLAG_IOPL2	= (1 << 13),
		FLAG_IOPL3	= FLAG_IOPL1+FLAG_IOPL2,

		FLAG_NESTEDTASK   = (1 << 14),
		FLAG_RESUME		  = (1 << 16),
		FLAG_V8086		  = (1 << 17),
		FLAG_ALIGNCHECK   = (1 << 18),
		FLAG_VIRTINT	  = (1 << 19),
		FLAG_VIRTINTPEN	  = (1 << 20),

		/// If settable the CPU supports the CPUID instruction
		FLAG_CPUID		  = (1 << 21),

		/// Mask for all reserved bits (should be 0)
		RESERVED_MASK 	= 0xFFFC8004
	};

   /// Template function to invoke a software interrupt (constant INT=0..255)
	template <u8 INT>
	inline static void interrupt( )
	{
		ASMVOLATILE( "int %0" :  :  "N"(INT) );
	}

   /// Enable/disable SW/HW interrupt processing
	inline static void allowInterrupts( bool b = true );

   /// Reads the current value of the EFLAGS register
	inline static flags_t EFLAGS()
	{
		flags_t flags;
		ASMVOLATILE(
			"pushfl  \n"
			"popl %0	\n"
			: "=r"(flags) );
		return flags;
	}

   /// Sets the value of the EFLAGS register
	inline static flags_t setEFLAGS( flags_t flags )
	{
		ASMVOLATILE(
			"pushl %0   \n"
			"popfl      \n"
         : "=r"(flags)
			: "0"(flags) );
		return flags;
	}


   /// Checks if interrupts are currently enabled (>0) or disabled(0)
	inline static u32 interruptsEnabled() CONSTFUNC
	{
		// could cache the flag
		return (Processor::EFLAGS() & FLAG_INT);
	}

   /// Reads the current value of the ESP register
	inline static u32* ESP() CONSTFUNC
	{
		u32* r; ASMVOLATILE( "movl %%esp, %0" : "=r"(r) : : "%esp", "memory" ); return r;
	}

	inline static void setESP( u32* esp ) {
	    ASMVOLATILE( "movl %0, %%esp" : : "r"(esp) : "%esp", "memory" );
	}

   /// Reads the current value of the EBP register
	inline static u32 EBP()
	{
		u32 r; ASMVOLATILE( "movl %%ebp, %0" : "=r"(r) ); return r;
	}

    /// Reads the current value of the EBX register
	inline static u32 EBX()
	{
		u32 r; ASMVOLATILE( "movl %%ebx, %0" : "=r"(r) ); return r;
	}

	/// Reads the current value of the EBX register
    inline static u32 EDX()
    {
        u32 r; ASMVOLATILE( "movl %%edx, %0" : "=r"(r) ); return r;
    }

    /// Reads the current value of the ESI register
	inline static u32 ESI()
	{
		u32 r; ASMVOLATILE( "movl %%esi, %0" : "=r"(r) ); return r;
	}

    /// Reads the current value of the EDI register
	inline static u32 EDI()
	{
		u32 r; ASMVOLATILE( "movl %%edi, %0" : "=r"(r) ); return r;
	}


   /// Reads the current value of the Code Segment selector
	inline static selector_t CS()
	{
		selector_t r;
		ASMVOLATILE ( "movw %%cs, %%ax" : "=a"(r) );
		return r;
	}

	inline static selector_t DS()	// actually Selector, perhaps move here ?
	{
		selector_t r;
		ASMVOLATILE ( "movw %%ds, %%ax" : "=a"(r) );
		return r;
	}

	inline static selector_t ES()	// actually Selector, perhaps move here ?
	{
		selector_t r;
		ASMVOLATILE ( "movw %%es, %%ax" : "=a"(r) );
		return r;
	}

	inline static selector_t SS()	// actually Selector, perhaps move here ?
	{
		selector_t r;
		ASMVOLATILE ( "movw %%ss, %%ax" : "=a"(r) );
		return r;
	}

   inline static u32 CR2()
   {
      u32 r;
      ASMVOLATILE ( "movl %%cr2, %%eax" : "=a"(r) );
      return r;
   }

	inline static u32 CR3()
	{
		u32 r;
		ASMVOLATILE ( "movl %%cr3, %%eax" : "=a"(r) );
		return r;
	}

	inline static u32 CR4()
	{
		u32 r;
		ASMVOLATILE ( "movl %%cr4, %%eax" : "=a"(r) );
		return r;
	}

	inline static void setCR4( u32 val )
	{
		ASMVOLATILE ( "movl %%eax, %%cr4" : "=a"(val) : "0"(val) );
	}


   /// Generates an architecturally defined (P6) invalid opcode, UD2
   inline static void generateInvalidOpcode() {
      ASMVOLATILE( ".byte 0xf \n .byte 0xb" );
   }

   /**
    * Checks if the CPU supports a local APIC, and enables it if possible
    */
   static bool enableLocalAPIC();

   /**
    * Checks if CPU supports any of the following features, and enables them:
    *  - Global pages (PGE)
    *  - Page Address Extensions (PAE), including NX bit support
    */
   static void enableNiceFeatures();


#ifdef DEBUG
   static void printStackTrace( u32 dwords );
#endif

#ifdef STATISTICS
	static unsigned getIntDisabledFraction();

private:
	static u64 int_off;
	static u64 int_disabled_ticks;
#endif

private:
   // see e.g. ftp://download.intel.com/design/pentium4/manuals/24547212.pdf P745
   enum E_MSR {
      MSR_IA32_APIC_BASE   =  0x1B,    // Starting from P6 family

      // and many more
   };

   /// Used for CPUID
   struct REGS {
      u32 eax, ebx, ecx, edx;
   };

   // see http://www.sandpile.org/ia32/cpuid.htm
   enum E_CPUID_OPCODES {
      E_GetVendorID           = 0,  //< gets max. supported level & vendor Id
      E_GetFeatures           = 1,  //< type, family, stepping
      E_GetConfigDescriptor   = 2,  //< Configuration descriptor
      E_GetSerialNumber       = 3,  //< #SN only if enabled

      __EXT__                  = 0x80000000,   // extended bit
      E_EXT_GetVendorID        = __EXT__ | 0,
      E_EXT_GetFeatures        = __EXT__ | 1,
      E_EXT_GetName0           = __EXT__ | 2,
      E_EXT_GetName1           = __EXT__ | 3,
      E_EXT_GetName2           = __EXT__ | 4,
      E_EXT_GetL1TLBDescriptor = __EXT__ | 5,
      E_EXT_GetL2Descriptor    = __EXT__ | 6,
      E_EXT_GetEPMInfo         = __EXT__ | 7,
      E_EXT_GetAdrSizeInfo     = __EXT__ | 8,

      // and some transmeta and mystery levels
   };

   inline static int supportsCPUID() {
      setEFLAGS( EFLAGS() | FLAG_CPUID );
      return (EFLAGS() & FLAG_CPUID);
   }
   static void cpuId( E_CPUID_OPCODES opcode, REGS& result );
   static E_CPUID_OPCODES getVendorId( char id[13] ); // 12 bytes + '0'
};

inline void Processor::allowInterrupts( bool b )
{
	// could cache the interrupt flag
	if (b)
	{
#ifdef STATISTICS
//		if (int_off) { int_disabled_ticks += TSC::read() - int_off; int_off = 0; }

#endif
        // TRACE_INT( b );  // difficult
		ASMVOLATILE( "sti" );
	} else {
		ASMVOLATILE( "cli" );
        // TRACE_INT( b );
#ifdef STATISTICS
//		int_off = TSC::read();
#endif
	}
}

}   // namespace

#endif
