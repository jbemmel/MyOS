#ifdef INCLUDE_PHASE

#include "mem.h"

/// In/out parameters to VM86 (BIOS) calls
struct VM86REGS {
   u16 di, si, bp, sp;  // sp will be ignored
   u16 bx, dx, cx, ax;

   u16 ds,es;

   // Constructor, clears all registers to 0
   VM86REGS() { clear(); }
   
   void clear() { memset_aligned( this, 0, sizeof(VM86REGS) ); }

} PACKED;

#else

#endif
