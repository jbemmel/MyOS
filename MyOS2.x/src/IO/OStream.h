#ifndef OSTREAM_H
#define OSTREAM_H

#include "IOStream.h"
#include "UUID.h"
// #include "Name.h"

namespace MyOS {
namespace IO {

/// The interface for output streams
class OStream : public IOStream
{
private:
	// Input methods
   virtual myos_result_t read( buffer& toBuffer, flags_t options ) { return E_MYOS_E_NOINTERFACE; }
   virtual const buffer getBuffer() const { return buffer(0,0); }

   // (for now) unsupported methods
   virtual bool setDelimiter( char delim ) { return false; }

public:
   // Output methods
   virtual s32 printf( const char* format, ... ) = 0;
   virtual s32 print( u64 number ) = 0;
   virtual s32 putBuffer( const buffer& buf ) = 0;
   virtual s32 put( char c ) = 0;
   virtual s32 flush() = 0;
};

inline OStream& operator<<( OStream& o, const char* msg)
{
   o.printf(msg);
   return o;
}

inline OStream& operator<<( OStream& o, const myos_uuid_t& uuid )
{
   o.putBuffer( buffer( (u8*) uuid.asString(),sizeof(myos_uuid_t)) );
   return o;
}

/*
inline OStream& operator<<( OStream& o, const myos_name_t& name )
{
   o.putBuffer( buffer( (u8*) name.name, name.length) );
   return o;
}
*/

inline OStream& operator<<( OStream& o, u64 n)
{
   o.print(n);
   return o;
}


}}	// namespace

#endif
