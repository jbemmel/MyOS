#ifndef IOPORT_H
#define IOPORT_H

/**
 *		General documentation		
 *
 *		This file defines templates to be used for defining and accessing IO ports
 *	  On the x86, an IO port has a 16-bit offset and can be 8, 16 or 32 bits wide
 *
 *		Some devices (e.g. PCI devices) allow their IO ports to be configured dynamically. This means
 *	  the actual index used may vary, and is not known at compile time. Other devices (e.g. old PC-
 *		inherited devices like the PIC or PIT ) have hardwired ports at fixed addresses
 *
 *		The advantages of defining IO ports in this way (rather than using direct inp, inw, etc) are:
 *		- Better code readibility, more O.O.
 *	    - Smaller chance for mistakes (e.g. writing 8bit to a 16bit port)
 *		- you can define read-only IO ports as 'const' !
 *		- Generated code is the same
 */

#include "types.h"
#include "asm.h"

#ifndef __GNUC__
#pragma warning(disable:4101)
#endif

//	Note: GCC can do the optimization (port<0xff) => in port, al (directly)

//
//	For some stupid reason, these can only be declared volatile when done like this
//
static inline void outb( u16 port, u8 data )
{
	ASMVOLATILE( "outb %1, %0" :  : "Nd"(port), "a"(data) );
}

static inline void outw( u16 port, u16 data )
{
	// GCC 3.0.1 does not accept 'ax' for data
	ASMVOLATILE( "outw %1, %0" :  : "Nd"(port), "a"(data) );
}

static inline void outl( u16 port, u32 data )
{
	ASMVOLATILE( "outl %1, %0" :  : "Nd"(port), "a"(data) );
}

static inline void insb( u16 port, u8* data, size_t bytes )
{
	u32 d0,d1;
   ASMVOLATILE(
      "cld; rep insb"
      : "=&D"(d0), "=&c"(d1)
      : "Nd"(port), "0"(data), "1"(bytes) );
}

static inline void insw( u16 port, u16* data, size_t bytes )
{
	u32 d0,d1;
   ASMVOLATILE(
      "cld; rep insw"
      : "=&D"(d0), "=&c"(d1)
      : "Nd"(port), "0"(data), "1"(bytes/2) );
}

static inline void insl( u16 port, u32* data, size_t bytes )
{
	u32 d0,d1;
   ASMVOLATILE(
      "cld; rep insl"
      : "=&D"(d0), "=&c"(d1)
      : "Nd"(port), "0"(data), "1"(bytes/4) );
}

static inline void outsb( u16 port, u8* data, size_t bytes )
{
	u32 d0,d1;
   ASMVOLATILE(
      "cld; rep outsb"
      : "=&S"(d0), "=&c"(d1)
      : "Nd"(port), "0"(data), "1"(bytes) );
}

static inline void outsw( u16 port, u16* data, size_t bytes )
{
	u32 d0,d1;
   ASMVOLATILE(
      "cld; rep outsw"
      : "=&S"(d0), "=&c"(d1)
      : "Nd"(port), "0"(data), "1"(bytes/2) );
}

static inline void outsl( u16 port, u32* data, size_t bytes )
{
	u32 d0,d1;
	ASMVOLATILE(
      "cld; rep outsl"
      : "=&S"(d0), "=&c"(d1)
      : "Nd"(port), "0"(data), "1"(bytes/4) );
}


static inline void _out2( u16 port, u32 lo_hi )
{
	ASMVOLATILE(
		"outb	%%al, %0	\t\n"
		"movb	%%ah, %%al	\t\n"
		"outb	%%al, %0	\t\n"
		:
		: "Nd"(port), "a"(lo_hi) );
}		

static inline u8 inb( u16 port )
{
	u8 data;
	ASMVOLATILE( "inb %1, %0" : "=a"(data) : "Nd"(port) );   // =al triggers bug
	return data;
}

static inline u16 inw( u16 port )
{
	u16 data;
	ASMVOLATILE( "inw %1, %0" : "=a"(data) : "Nd"(port) );
	return data;
}

static inline u32 inl( u16 port )
{
	u32 data;
	ASMVOLATILE( "inl %1, %0" : "=a"(data) : "Nd"(port) );
	return data;
}


/// 8 bit IOPort class, portnumber can be chosen at runtime (rather than compiletime)
class IOPort8_v		
{
public:
	inline IOPort8_v( u16 p ) : port(p) {}

	inline operator u8() const 		{ return inb(port); }	
	inline void operator =( u8 b ) 	{ outb(port,b); }
   inline void readString( u8* data, size_t bytes ) const { insb(port,data,bytes); }
   inline void writeString( u8* data, size_t bytes ) { outsb(port,data,bytes); }

   // sometimes convenient to set after constructor
   inline void setPort( u16 p ) { port=p; }
   inline u16 getPort() const { return port; }

   // read/write with offset from base address
   inline u8 readAtOffset( u16 offset ) const { return inb(port+offset); }
   inline void writeAtOffset( u16 offset, u8 w ) { outb(port+offset,w); }

   inline void readStringAtOffset( u8* data, size_t bytes, u16 offset ) {
      insb(port+offset,data,bytes);
   }
   inline void writeStringAtOffset( u8* data, size_t bytes, u16 offset ) {
      outsb(port+offset,data,bytes);
   }

private:
	u16 port;
};

/// 16 bit IOPort class, portnumber can be chosen at runtime (rather than compiletime)
class IOPort16_v
{
public:
	inline IOPort16_v( u16 p ) : port(p) 	{}
	inline operator u16() const 				{ return inw(port); }	
	inline void operator =( u16 w ) 			{ outw(port,w); }
   inline void readString( u16* data, size_t bytes ) { insw(port,data,bytes); }
   inline void writeString( u16* data, size_t bytes ) { outsw(port,data,bytes); }

   // sometimes convenient to set after constructor
   inline void setPort( u16 p ) { port=p; }
   inline u16 getPort() const { return port; }

   // read/write with offset from base address
   inline u16 readAtOffset( u16 offset ) const { return inw(port+offset); }
   inline void writeAtOffset( u16 offset, u16 w ) { outw(port+offset,w); }

private:
	u16 port;
};

/// 32 bit IOPort class, portnumber can be chosen at runtime (rather than compiletime)
class IOPort32_v
{
public:
	inline IOPort32_v( u16 p ) : port(p) {}
	inline operator u32() const          { return inl(port); }	
	inline void operator =( u32 l )      { outl(port,l); }
   inline void readString( u32* data, size_t bytes ) const { insl(port,data,bytes); }
   inline void writeString( u32* data, size_t bytes ) { outsl(port,data,bytes); }

   // sometimes convenient to set after constructor
   inline void setPort( u16 p ) { port=p; }
   inline u16 getPort() const { return port; }
private:
	u16 port;
};

// Port mode: read-write(default), read-only, write-only
// TODO: Apply this consistently here
enum E_MODE { E_RW=0, E_RO, E_WO };

/// 8 bit IOPort class, portnumber must be fixed (known) at compiletime
template <u16 port, E_MODE mode = E_RW>
class IOPort8
{
public:
	inline IOPort8() {}

	// Support syntax like IOPort8<n> p = x;
	inline IOPort8( u8 b ) { if (mode!=E_RO) outb(port,b); }

	inline operator u8() const 			{ return inb(port); }
	inline void operator =( u8 b ) 		{ outb(port,b); }
	inline void out2( u32 lo_hi ) 		{ _out2(port,lo_hi); }
	inline void readString( u8* data, size_t bytes ) const { insb(port,data,bytes); }
	inline void writeString( u8* data, size_t bytes ) { outsb(port,data,bytes); }
};

/// Same as above, read only
template <u16 port>
class IOPort8_R
{
public:
	inline IOPort8_R() {}
	inline operator u8() const 			               { return inb(port); }
   inline void readString( u8* data, size_t bytes )   { insb(port,data,bytes); }
};

/// Same as above, write only
template <u16 port>
class IOPort8_W
{
public:
	inline void operator =( u8 b ) 		{ outb(port,b); }
	inline void out2( u32 lo_hi ) 		{ _out2(port,lo_hi); }
	inline void writeString( u8* data, size_t bytes ) { outsb(port,data,bytes); }
};


/// 16 bit IOPort class, portnumber must be fixed (known) at compiletime
template <u16 port>
class IOPort16
{
public:
	inline operator u16() const 		{ return inw(port); }	
	inline void operator =( u16 w )		{ outw(port,w); }
	inline void readString( u16* data, size_t bytes ) const  { insw(port,data,bytes); }
	inline void writeString( u16* data, size_t bytes ) { outsw(port,data,bytes); }
};

/// Same as above, read only
template <u16 port>
class IOPort16_R
{
public:
	inline operator u16() const 		                  { return inw(port); }	
   inline void readString( u16* data, size_t bytes ) const { insw(port,data,bytes); }
};

/// Same as above, write only
template <u16 port>
class IOPort16_W
{
public:
	inline void operator =( u16 w ) 		           { outw(port,w); }
	inline void writeString( u16* data, size_t bytes ) { outsw(port,data,bytes); }
};

/// 32 bit IOPort class, portnumber must be fixed (known) at compiletime
template <u16 port>
class IOPort32
{
public:
	inline operator u32() const      { return inl(port); }	
	inline void operator =( u32 l )  { outl(port,l); }
	inline void readString( u32* data, size_t bytes ) const  { insl(port,data,bytes); }
	inline void writeString( u32* data, size_t bytes ) { outsl(port,data,bytes); }
};

/// Same as IOPort32, read only
template <u16 port>
class IOPort32_R
{
public:
	inline operator u32() const      { return inl(port); }
   inline void readString( u32* data, size_t bytes )   { insl(port,data,bytes); }
};

/// Same as IOPort32, write only
template <u16 port>
class IOPort32_W
{
public:
	inline void operator =( u32 l )  { return outl(port,l); }
   inline void writeString( u32* data, size_t bytes ) { outsl(port,data,bytes); }
};

#endif

