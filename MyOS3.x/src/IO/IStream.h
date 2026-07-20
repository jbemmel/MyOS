/*
 *	There are 2 ways to model IOStream, IStream, OStream:
 *	1)	Derive IOStream from both IStream & OStream (multiple ih)
 *	2)	Derive IStream & OStream from IOStream, privatize invalid methods
 *
 *	I choose option (2) since multiple inheritance is cumbersome
 */

#ifndef ISTREAM_H
#define ISTREAM_H

#include "IOStream.h"
#include "myosresult.h"     // myos_result_t
#include "types.h"          // s32

namespace MyOS {

/// The namespace for IO related classes
namespace IO {

/// Abstract base class for all inputstream classes
class IStream : public IOStream
{
public:
    // Generic methods

    /**
     *   Seek a given position in the stream
     *   @param offset     -  The offset (+ or -) to seek
     *   @param isRelative -  If true, offset will be interpreted as relative to current pos
     *
     *   @return true if succeeded, false otherwise
     */
   virtual bool seek( s32 offset, bool isRelative=false ) = 0;

    /***** ##### INPUT METHODS ##### *****
        I dont want to use exceptions, so...
    */

    /**
     * Reads a single byte from this stream (convenience method for read(1))
     * @param byte - Location to receive the result byte
     *
     * @return see IOStream#read
     * @note for efficiency reasons, use multi-byte #read() whenever possible
     */
    inline myos_result_t getByte( u8& byte ) { buffer b( &byte, 1); return read( b, 0 ); }

    /**
     * Reads a single word (2 bytes, little-endian) from this stream (convenience method for read(2))
     * @param word - Location to receive the result word
     *
     * @return see IOStream#read
     * @note for efficiency reasons, use multi-byte #read() whenever possible
     */
    inline myos_result_t getWord( u16& word ) { buffer b( (u8*) &word, 2); return read( b, 0 ); }

    /**
     * Reads a single dword (4 bytes, little-endian) from this stream (convenience method for read(4))
     * @param dword - Location to receive the result dword
     *
     * @return see IOStream#read
     * @note for efficiency reasons, use multi-byte #read() whenever possible
     */
    inline myos_result_t getDword( u32& dword ) { buffer b( (u8*) &dword, 4); return read( b, 0 ); }

	/** @see IOStream#read */
	virtual myos_result_t read( buffer& toBuffer, flags_t options ) = 0;

	/** @see IOStream#getBuffer */
	virtual const buffer getBuffer() const = 0;

   /***** ##### OUTPUT METHODS, INVALID #### *****/
private:
   /// Not supported, @return -1
   virtual s32 printf( const char* format, ... )   { return -1; }

   /// Not supported, @return -1
   virtual s32 print( u64 number )                 { return -1; }

   /// Not supported, @return -1
   virtual s32 putBuffer( const buffer& buf )      { return -1; }

   virtual s32 flush() { return -1; }
};

}}	// namespace

#endif
