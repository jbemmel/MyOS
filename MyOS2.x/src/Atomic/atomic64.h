/**
 *	Encapsulating class for 64 bits data. This can be very compiler specific
 */

#ifndef ATOMIC64_H
#define ATOMIC64_H

#include "defs.h"
#include "types.h"

/// An atomic 64 bit class
class atomic64
{	
public:
	inline atomic64() 			            { bits64 = 0; }
	//inline atomic64( const atomic64 &d ) 	{ bits64 = d.bits64; }

	inline atomic64( u64 n ) { *this = *((atomic64 *) &n); }

	// note : this generates quite a lot of code !!
	inline bool CmpSwap( const atomic64& old, const atomic64& new_value )	// old can be *this -> no const
	{
		bool r;
		ASMVOLATILE(
			"cmpxchg8b	(%1)	\t\n"
			"setz		%0		\t\n"
			: 	"=r"(r)
			: 	"r" (this), 	
				"a"(old.lohi.lo), "d"(old.lohi.hi),
				"b"(new_value.lohi.lo), "c"(new_value.lohi.hi)
			);
		return r;
	}

	inline operator u64() { return *this; }

    /// Atomically assigns a new value  @todo fix this
	inline atomic64& operator =( const atomic64& val )
	{
		while ( CmpSwap( *this, val) == false);
		return *this;
	}

   /// Atomically sets this 64 bit value to val
   inline void setAtomic( const u64& val )
   {
      this->operator=(val);
   }

    /// Clear atomically    @todo fix this
   inline void clear() {
		while ( CmpSwap( *this, 0) == false);
   }

private:

	union {
		u64 bits64;

        /// 64bit read as 2*32
        struct {
			u32 lo;	// little endian !
			u32 hi;
		} lohi;
	};
};

/// Template to enhance any 64 bit type with atomic operations
template <typename T64>	
class ATOMIC64 : public atomic64				// helper class to treat any 64 bit class as a atomic64
{
public:
	inline ATOMIC64() {}
	ATOMIC64( const T64 &s ) : atomic64( (u64 &) s ) 	{}

	operator T64&() 					{ return *((T64 *) this); }
	operator const T64&() const   { return *((const T64 *) this); }

	void assignAtomic( const T64& t ) { atomic64::operator=( (atomic64&) t ); }
    	
	// type-cast functions of atomic64 :(
};




#endif
