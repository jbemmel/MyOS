/// Inline assembly related definitions

#ifndef ASM_H
#define ASM_H

// EXPECT can provide branch prediction as in if (EXPECT(x,1)) {

#ifdef __GNUC__
#  define ASM __asm__
#  define ASMVOLATILE __asm__ __volatile__
#  define RARELY(x) (__builtin_expect((int)(x),false))
#  define OFTEN(x) (__builtin_expect((int)(x),true))
#else
#  define ASM(X)
#  define ASMVOLATILE(X)
#  define RARELY(x) (x)
#  define OFTEN(x) (x)
// disable warning about too many actual parameters for macros
#pragma warning(disable:4002)
#endif

#endif
