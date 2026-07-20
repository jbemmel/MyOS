/**
 *	Encapsulating class for 32 bits atomic data. This can be very compiler specific
 */

#ifndef ATOMIC32_H
#define ATOMIC32_H

#include "defs.h"
#include "types.h"
#include "asm.h"
#include "atomic.h"

namespace MyOS
{

/// An atomic 32bit class
class atomic32
{
public:
    enum { ZERO = 0 };

    // initialize
    atomic32(const atomic32 &d);

    // Caution: when used in arrays, this gets called !
    inline atomic32(s32 d = 0);

    // read / write
    inline volatile atomic32& operator =(const atomic32 &d) volatile;
    inline operator s32() volatile const
    {
        return data;
    }

    // modify
    inline s32 operator ++() volatile; // prefix
    inline const s32 operator++(int) volatile; // postfix
    inline s32 operator--() volatile; // prefix
    inline const s32 operator--(int) volatile; // postfix
    inline s32 operator +=(s32) volatile;

    inline void operator |=(u32) volatile;
    inline void operator &=(u32) volatile;

    // returns old value, new_value==atomic32 gives internal_compiler_error
    inline u32 Replace(u32 new_value) volatile;

    // @return True iff replacement succeeded
    inline int CmpXchg(u32 cmp, const u32 newval) volatile;

    inline s32 CmpXchg_retold(u32 cmp, const u32 newval) volatile;

    inline u32 ReadAdd(s32 add) volatile;

    inline void set( s32 v ) volatile { this->data = v; }

    inline void inc() volatile;
    inline void dec() volatile;
    inline bool decAndTest() volatile;

private:
    volatile s32 data; /// signed, more intuitive for operators ++ and --
};

/// Helper template to treat any 32bit data as atomic
template<typename T32> class ATOMIC32 : public atomic32
{
public:
    inline ATOMIC32(u32 i = 0) throw() : atomic32(i )
    {
    }

    inline operator T32() volatile const
    {
        return *((T32*) this);
    }

    inline volatile T32& operator =(const T32 &v) volatile
    {
        return (T32&) atomic32::operator = ( (const u32 &) v);
    }

    // type-cast functions of atomic32 :(
    inline u32 Replace( u32 new_value ) volatile
    {
        return (u32) atomic32::Replace( new_value );
    }
};

// inline functions
inline atomic32::atomic32( const atomic32 &d ) : data( d.data )
{}
inline atomic32::atomic32( s32 d ) : data( d )
{}

inline volatile atomic32& atomic32::operator =( const atomic32 &d ) volatile
{
    data = d.data; return *this;
}

// For XCHG
struct __xchg_dummy2
{   unsigned long a[100]; };
#define __xg2(x) ((struct __xchg_dummy2 *)(x))

inline u32 atomic32::Replace( u32 new_value ) volatile
{
    // has implicit LOCK prefix
    ASMVOLATILE( "xchg %0, %2"
            : "=r" (new_value)
            : "0" (new_value), "m"(*__xg2(this))
            : "memory" );
    return new_value;
}

/**
 * Compare this value with another value, and replace it if equal
 * @param cmp     : Value to compare with
 * @param newval  : Value to replace if this==cmp
 *
 * @return True iff value was replaced sucessfully
 */
inline int atomic32::CmpXchg( u32 cmp, const u32 newval ) volatile
{
    ASMVOLATILE(
            LOCK "cmpxchgl %1, %2 \r\n"
            "setz %%al"
            : "=a"(cmp)
            : "q"(newval), "m"(*__xg2(this)), "0"(cmp)
            : "memory"
    );
    return cmp; // == true iff succeeded
}

inline s32 atomic32::CmpXchg_retold( u32 cmp, const u32 newval ) volatile
{
    s32 prev;
    ASMVOLATILE(
         LOCK "cmpxchgl %1,%2"
         : "=a"(prev)
         : "q"(newval), "m"(*__xg2(this)), "0"(cmp)
         : "memory");
    return prev;
}


/// Returns the value before adding
inline u32 atomic32::ReadAdd( s32 add ) volatile
{
    // has implicit LOCK prefix
    ASMVOLATILE(
            "xadd %1, (%2)"
            : "=r"(add)
            : "0"(add), "r"(this)
            : "memory"
    );
    return add;
}

inline s32 atomic32::operator ++() volatile
{   return ReadAdd(1) + 1;}
inline const s32 atomic32::operator ++(int) volatile
{   return ReadAdd(1);}

inline s32 atomic32::operator --() volatile
{   return ReadAdd(-1) - 1;}
inline const s32 atomic32::operator --(int) volatile
{   return ReadAdd(-1);}

inline void atomic32::inc() volatile
{
    // data |= n;
    ASMVOLATILE(
            LOCK "incl %0"
            : "=m" (this->data)
            : "m" (this->data));
}

inline void atomic32::dec() volatile
{
    ASMVOLATILE(
            LOCK "decl %0"
            : "=m" (this->data)
            : "m" (this->data));
}

inline bool atomic32::decAndTest() volatile
{
    u32 ret;
    ASMVOLATILE(
            LOCK "decl %1; setz %%al"
            : "=a"(ret), "=m" (this->data)
            : "m" (this->data));
    return ret;
}

inline void atomic32::operator |=(u32 n) volatile
{
    // data |= n;
    ASMVOLATILE(
            LOCK "orl %1,%0"
            :"=m" (this->data)
            :"ir" (n), "m" (this->data));
}

inline void atomic32::operator &=(u32 n) volatile
{
    // data &= n;
    ASMVOLATILE(
            LOCK "andl %1,%0"
            :"=m" (this->data)
            :"ir" (n), "m" (this->data));
}

inline s32 atomic32::operator +=(s32 n) volatile
{
    return ReadAdd(n)+n;
}

}

#endif

