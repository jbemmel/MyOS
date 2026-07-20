#ifndef BufWriter_H
#define BufWriter_H

#include "buffer.h"
#include "stdarg.h"

namespace MyOS {
namespace IO {

/**
 * Output writer class that performs 'printf' functionality
 *
 * The class maintains a pointer to the current buffer & last index in this buffer, and avoids
 * writing past the buffer space
 */
class BufWriter
{   
public:
   /// Constructor
   /**
    * @note To avoid buffer overflow issues, make sure 'space' is large enough to hold
    * IOStream::PRINTF_MAX_FORMAT_LENGTH
    */
   BufWriter( const buffer& space );

   // convenience puts
   inline void putByte( u8 b ) { if (fits(sizeof(u8))) { *index++ = b; } }
   void putWord( u16 w );
   void putDword( u32 d );
   void putString( const char* sz );

   // returns #bytes actually written
   size_t putBuffer( const buffer& buf, size_t amount );

   inline const char* getString() const { return (const char*) obuf.getData(); }
   inline const buffer& getBuffer() const { return obuf; }
   inline size_t getSpace() const { return (obuf.getEnd() - (u8*)index); }

   inline size_t getBytesWritten() const { return (u8*) index - obuf.getData(); }
   inline const buffer getAsBuf() const { return buffer( obuf.getData(), getBytesWritten() ); }

   s32 va_snprintf( const char* fmt, va_list pvar);

   inline void flush() { index = (char*) obuf.getData(); }

   // setting buffer resets index
   inline void setBuffer( const buffer& b ) {
      obuf = b;
      flush(); // reset index
   }

   inline bool fits( size_t bytes ) const { 
       return bytes <= (size_t) (obuf.getEnd() - (u8*)index); 
   }
   
private:
   buffer obuf;   // make a copy! Otherwise stack-based variables...
   char* index;
};

}} // namespace

#endif
