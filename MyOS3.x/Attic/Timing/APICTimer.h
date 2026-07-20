#ifndef APICTimer_H
#define APICTimer_H

#include "types.h"
#include "InterruptHandling/IInterruptHandling.h"
#include "MM/IVirtualMemory.h"

// see ftp://download.intel.com/design/pentium4/manuals/24547212.pdf
// section 8.5.4

namespace MyOS { namespace Timing {

using namespace InterruptHandling;
using MM::IVirtualMemory;
using MultiThreading::Thread;  
  
/**
 * Represents the loca APIC timer available from Pentiums
 */  
class APICTimer
{
   // TRAP handler, written in asm
   static void onTimer() ASMNAME( "APIC_onTimer" );

   // TRAP handler, C++ part
   static Thread* handleTimer() ASMNAME( "handleTimer" );
   
   // Some I/O registers, memory mapped      
   enum {
      APIC_BASE           = 0xFEE00000,  // Memory mapped base, programmable
   };

   // some utility functions, adopted from linux
   inline u8 getMaxLVT() const;

   enum E_DCR_VALUES {
      E_DIV_1   = 0xB,  // 1011 binary, fastest
      E_DIV_2   = 0x0,  // 0000 binary
      E_DIV_4   = 0x1,  // 0001 binary
      E_DIV_8   = 0x2,  // 0010 binary
      E_DIV_16  = 0x3,  // 0011 binary
      E_DIV_32  = 0x8,  // 1000 binary
      E_DIV_64  = 0x9,  // 1001 binary
      E_DIV_128 = 0xA,  // 1010 binary, slowest
   };

   enum E_LVT_BITS {
      E_MASK      = (1<<16),
      E_PERIODIC  = (1<<17),
   };

   // Static, addressed from static handler function
   struct LOCALAPIC;
   static volatile LOCALAPIC* apic;

public:
   bool init( IVirtualMemory& pm, IInterruptHandling& ih ) INITSECTION;     
};  
  
}} // namespace

#endif
