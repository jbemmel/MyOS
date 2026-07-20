#ifndef DEFS_H
#define DEFS_H

typedef unsigned int size_t;
#include "asm.h"    // ASM

#define INT_MAX      2147483647

/// Borrowed from linux, usable to check offsets in structures
#define OFFSET_OF(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define _OFFSET_OF(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

// Use this macro to define class constants
#define CLASSCONSTANT(type,name,value) enum { name = value }
// alternative: static const type name = value, but not all compilers support that

#ifdef COMPONENTCOMPILATION
// For containers, .bss should not be used -> explicit initializer
#  define INITINSTANCE =0
#else
#  define INITINSTANCE
#endif

#ifdef __GNUC__
// GCC specific: put code in INIT section
#define STARTSECTION __attribute__ ((section("START")))
#define MAGICSECTION __attribute__ ((section("MAGIC")))
#define INITSECTION __attribute__ ((section(".text.init")))
#define CODESECTION __attribute__ ((section(".text")))
#define DATASECTION __attribute__ ((section(".data")))
#define BSSSECTION __attribute__ ((section(".bss")))
#define TABLESECTION __attribute__ ((section(".table")))
#define NOSECTION __attribute__ ((section(".nosection")))
#define UNCOMPRESSEDSECTION __attribute__ ((section(".rodata.UNCOMPRESSED")))

#define NORETURN __attribute__ ((noreturn))
#define NOTHROW __attribute__ ((nothrow))
#define REGPARM(n) __attribute__ ((regparm(n)))
#define PACKED __attribute__ ((packed))
#define UNUSED __attribute__((unused))
#define CONSTFUNC __attribute__((const))
#define ASMNAME(n) asm(n)

// These should be in DWORD class

// this forces in-memory and
#define mem_write_and(var,value) __asm__( "andl %1, %0" :  :  "m"(var), "r"(value) )

// this forces in-memory or
#define mem_write_or(var,value)	__asm__( "orl %1, %0" :  :  "m"(var), "r"(value) )

// this beeps ? from linux page.h
#define beep() __asm__ __volatile( ".byte 0xf, 0xb" );

#else

#define STARTSECTION
#define MAGICSECTION
#define INITSECTION
#define CODESECTION
#define DATASECTION
#define NORETURN
#define REGPARM(n)
#define BSSSECTION
#define INITSECTION
#define NOSECTION
#define TABLESECTION
#define UNCOMPRESSEDSECTION
#define PACKED
#define UNUSED
#define CONSTFUNC
#define ASMNAME(n)

#define mem_write_and(var,value)	var&=value
#define mem_write_or(var,value)		var|=value

// Turn off some warnings
#pragma warning(disable:4290)	// exception specs ignored

#endif


// placement new    in <memory>
// #ifndef __NEW__
// inline void *operator new( size_t, void *location)	{ return location; }
// #endif

#ifdef __cplusplus

// fast division
template <unsigned divisor>
inline int FASTIDIV( int n )
{
	// n / divisor == (n * (2^32/divisor)) >> 32, roughly equal to ((2^32-1) / divisor + 1) >> 32 ( == EDX !!! )
	// +1 is important! Example: 195001 / 195000 => FASTIDIV<195000>(195001) == 0 otherwise !
	int result;
	__asm__ ( "imul %3" : "=d" (result), "=a"(n) : "1" (n), "r" ( (0xffffffff / divisor) + 1 ) ); 	// eax clobbered !!
	return result;
}

// fast unsigned division
template <unsigned divisor>
inline unsigned FASTDIV( unsigned n )
{
	// n / divisor == (n * (2^32/divisor)) >> 32, roughly equal to ((2^32-1) / divisor + 1) >> 32 ( == EDX !!! )
	// +1 is important! Example: 195001 / 195000 => FASTIDIV<195000>(195001) == 0 otherwise !
   //
   // Found a slightly different version: a/b == (int)(((2^32)+b-1)/b) * (a) / (2^32)
   // Not sure if that is correct
   //
	int result;
	ASM ( "mul %3" : "=d" (result), "=a"(n) : "1" (n), "r" ( (0xffffffff / divisor) + 1 ) ); 	// eax clobbered !!
	return result;
}

#endif

// to be used with FASTIDIV above
inline int RECIPROCE( int divisor )
{
    // ASSERT( divisor, A_FATAL );
    return (0xffffffff/divisor)+1;
}

inline unsigned RECIPROCE_U( unsigned divisor )
{
    // ASSERT( divisor, A_FATAL );
    return (0xffffffff/divisor)+1;
}


// reciproce should be (0xffffffff / divisor)+1
inline int FASTIDIV( int n, int reciproce )
{
	int result;
	ASM( "imul %3" : "=d" (result), "=a"(n) : "1" (n), "r" ( reciproce ) );
	return result;
}

inline unsigned FASTDIV( unsigned n, unsigned reciproce )
{
	unsigned result;
	ASM( "mul %3" : "=d" (result), "=a"(n) : "1" (n), "r" ( reciproce ) );
	return result;
}


/// Divide, rounded up
inline int DIV_ROUND_UP( int x, int by )
{
    return ((x+by-1)/by);
}

// Takes 32-<SHIFT> MSB bits from p[0] | <SHIFT> LSB from p[1] 
template <unsigned char SHIFT>
inline static unsigned shr( const unsigned* p ) {
   unsigned r;
   __asm( "shrdl %3, %2, %0"
        : "=r"(r)
        : "0"(p[0]), "r"(p[1]), "N"(SHIFT)
   );
   return r;
}

// Takes 32-<SHIFT> LSB bits from p[0] | <SHIFT> MSB from p[1] 
template <unsigned char SHIFT>
inline static unsigned shl( const unsigned* p ) {
   unsigned r;
   __asm( "shldl %3, %2, %0"
        : "=r"(r)
        : "0"(p[0]), "r"(p[1]), "N"(SHIFT)
   );
   return r;
}


#ifdef __cplusplus

// This function must be defined in each component
extern "C" void __pure_virtual() NORETURN;
inline void __pure_virtual() {
    ASMVOLATILE( "int $255" );
    while(1);
}

// GCC 3.0.1 defines __cxa_pure_virtual instead
extern "C" void __cxa_pure_virtual() NORETURN;
// inline void __cxa_pure_virtual() { __pure_virtual(); }

// Used for cleanup of static variables
extern "C" int atexit(void (*function)(void));
// inline int atexit( void (*function)(void) ) { return -1; }

#endif

// Use for editable kernel parameters
// #define KERNELPARAM(type,name,val)
//   static type name ASMNAME( "KP_" ## type ## "_" ## name ) UNCOMPRESSEDSECTION = val

#endif		// DEFS_H
