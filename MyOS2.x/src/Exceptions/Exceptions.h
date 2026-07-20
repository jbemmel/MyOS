/**
 * This header makes it possible to inline certain exception-related calls ??
 * 
 * No, does not seem to work (GCC cannot inline, perhaps because the call is
 * implicit?)
 * 
 *
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "MultiThreading/IThread.h"
#include "Processor/Processor.h"

using MyOS::MultiThreading::IThread;
using MyOS::MultiThreading::Thread;
using MyOS::Processor;

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned _Unwind_Word __attribute__((__mode__(__word__)));

// This structure is allocated on the stack of the target function.
// This must match the definition created in except.c:init_eh.
struct SjLj_Function_Context
{
  // This is the chain through all registered contexts.  It is
  //   filled in by _Unwind_SjLj_Register.
  struct SjLj_Function_Context *prev;

  // This is assigned in by the target function before every call
  //   to the index of the call site in the lsda.  It is assigned by
  //   the personality routine to the landing pad index.
  int call_site;

  // This is how data is returned from the personality routine to
  // the target function's handler.
  _Unwind_Word data[4];

  // These are filled in once by the target function before any
  //   exceptions are expected to be handled.
  void* 
  // _Unwind_Personality_Fn
         personality;

  // JvB: Language specific data area
  void *lsda;

  void *jbuf[];
};

inline void
_Unwind_SjLj_Register (struct SjLj_Function_Context *fc)
{
   // DPRINTK( "\n_Unwind_SjLj_Register fc=%x", fc );
   // GCC cannot inline since this uses inline ASM?
   register Thread* cur; // = (Thread*) (((u32) Processor::ESP() | (_4KB-1))+1-sizeof(Thread));
   __asm__ (
      "movl %%esp,  %0   ;"
      "orl  $0xFFF, %0   ;"
      "addl %1  ,   %0   ;"
      : "=&r"(cur)
      : "i"( 1-sizeof(Thread) )
   );
   
   fc->prev = (SjLj_Function_Context*)cur->exception_fc;
   cur->exception_fc = fc;
}

#ifdef __cplusplus
}
#endif

#endif
*/
