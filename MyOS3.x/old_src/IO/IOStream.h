/*
 *	There are 2 ways to model IOStream, IStream, OStream:
 *	1)	Derive IOStream from both IStream & OStream (multiple ih)
 *	2)	Derive IStream & OStream from IOStream, privatize invalid methods
 *
 *	I choose option (2) since multiple inheritance is cumbersome
 */

#ifndef IOSTREAM_H
#define IOSTREAM_H

#include "myosresult.h"     // myos_result_t
#include "types.h"          // s32
#include "buffer.h"

namespace MyOS {

/// Contains all IO related classes
namespace IO {

/// Option flag bits for IOStream classes
enum E_IO_OPTION {

   /**
    * Read/write until the delimiter is seen or the stream is closed
    * Calls will block until the delimiter is available, if received data
    * will not fit into the receive buffer before that an error is returned
    */
   E_UNTIL_DELIMITER    = 0x00000001,

   /**
    * Suspend the reading thread until #size bytes are available, or the
    * stream is closed / end of stream is reached
    */
   E_BLOCK_UNTIL_AVAIL  = 0x00000002,
};

/// A bitmask of option bits (no options have been defined yet)
typedef u32 flags_t;

/// Abstract base class for all IO stream classes
/**
 * Streams are an abstraction for sequentially accessible data. The IOStream classes are responsible
 * for formatting the data (both text and binary) and use IOBuffer classes for temporary storage
 *
 * The IOStream classes will focus on minimal functionality that is needed in an OS Kernel.
 * Some functionality like extended formatting is more likely to be part of optional Components
 */
class IOStream
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

   /**
    * Returns a number indicating the current position in the stream
    * Can be used to return to a previous position using seek() or to count the number
    * of bytes processed between two getMarker calls
    *
    * @return current position, or -1 if not supported
    */
   virtual s32 getMarker() const = 0;

   /**
    * Sets a delimiter char that causes the stream to be closed when encountered
    * This method is usually used by the receiver of the (input) stream
    *
    * Note: the delimiter is not removed from the stream (i.e. will be read)
    *
    * @param delim : Delimiter to set, e.g. '\n' or '\026'(^Z)
    * @return false iff not supported
    */
   virtual bool setDelimiter( char delim ) = 0;

   /**
    * Closes the IOStream
    * After closing, no more IO is possible on this stream
    */
   virtual myos_result_t close() = 0;

    /***** ##### INPUT METHODS ##### *****/

   /**
    * Reads an amount of bytes into a buffer
    *
    * This method will normally return immediately, reading as many bytes (>=0)
    * as are available and will fit in toBuffer.
    *
    * @param toBuffer -   Buffer to receive the data
    * @param options  -   bitmask of E_IO_OPTION flagbits:
    *                     E_UNTIL_DELIMITER if delimiter should be used
    *                     E_BLOCK_UNTIL_AVAIL if call should block
    *
    * @return number of bytes (>=0) if succesful
    * @return E_UNSUPPORTED_OPTION if a flag is set for an unsupported option
    * @return E_DELIM_NOT_SET if flag E_UNTIL_DELIMITER is set but no delimiter
    * @return E_BUF_TOO_SMALL if flag E_UNTIL_DELIMITER is set but no delimiter
    *                             was seen before sizeof(buffer) bytes
    */
   virtual myos_result_t read( buffer& toBuffer, flags_t options ) = 0;

   /**
    * Returns the current contents as a read-only buffer
    */
   virtual const buffer getBuffer() const = 0;

    /***** ##### OUTPUT METHODS ##### *****/

   /**
    * Forces pending write buffers to be flushed (also needed for input?)
    */
   virtual s32 flush() = 0;

   /**
    * Prints a formatted string to this stream
    *
    * @param format  -  The format string to use (a la C's 'printf')
    * @param ...     -  Any arguments mentioned in the format string
    *
    * The following format characters are recognised:
    * <br>%%c - char
    * <br>%%d - signed 32bit number
    * <br>%%s - string ("null" in case of null ptr argument), @see PRINTF_MAX_FORMAT_LENGTH
    * <br>%%u - unsigned 32bit number
    * <br>%%x - hexadecimal 32bit number, formatted beginning with '0x'
    *
    * Non-standard extensions:
    * <br>%%b - bool ('0' or '1')
    * <br>%%n - pointer(!) to myos_name_t (nothing added if bufferfull)
    * <br>%%r - repeat next character n times, n from '...' (@see PRINTF_MAX_FORMAT_LENGTH)
    * <br>%%X - 2 hexdigit unsigned 8bit number
    *
    * @return <br>number of bytes written if success
    * @return <br>negative value on failure
    *
    * @note For now, no flags or options are supported
    */
   virtual s32 printf( const char* format, ... ) = 0;
   virtual s32 print( u64 number ) = 0;

   /**
    * Adds the contents of a buffer to the stream
    * @param buf  - Buffer to add
    * @return number of bytes added, or -1 if failed
    */
   virtual s32 putBuffer( const buffer& buf ) = 0;

   /// Artificial implementation limits
   enum E_LIMITS {
      PRINTF_MAX_FORMAT_LENGTH = 0x100,  ///< To avoid buffer overflow issues, limited %s and %r length
   };
};

}}	// namespace

#endif
