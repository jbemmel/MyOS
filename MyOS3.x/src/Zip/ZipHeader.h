#ifndef ZIPHEADER_H
#define ZIPHEADER_H

#include "types.h"
#include "buffer.h"
#include "myosresult.h" // error codes

namespace MyOS {
namespace Zip  {

/// Resultcodes for unzip, negative==error
enum E_RESULT {
   E_OK                             = 0,
   E_ERROR                          = 0x80000000, // sign bit
   E_BAD_MAGIC_NUMBER               = E_ERROR | 1,
   E_INVALID_COMPRESSION_METHOD     = E_ERROR | 2,
   E_ENCRYPTED_INPUT                = E_ERROR | 3,
   E_MULTIPART_INPUT                = E_ERROR | 4,
   E_INVALID_FLAGS                  = E_ERROR | 5,

   E_INVALID_COMPRESSION1           = E_ERROR | 6,
   E_INVALID_COMPRESSION2           = E_ERROR | 7,
   E_INVALID_COMPRESSION_OTHER      = E_ERROR | 8,

   E_DATA_ERROR                     = E_ERROR | 9,
   E_CRC_ERROR                      = E_ERROR | 10,
   E_LENGTH_MISMATCH                = E_ERROR | 11,
   E_BAD_HEADER                     = E_ERROR | 12,

   /// @todo standard error codes also possible
   E_NOMEM                          = E_MYOS_ENOMEM,

   /// internal codes
   E_LAST_BLOCK                     = 0xaabbccdd,
};

/**
 *  Represents the header in a zipped datablock
 *
 */
class ZipHeader
{
   u32 timestamp;
   const char* originalName;   // points into buffer -> not long valid !
   buffer zipdata;             // points to start of data after parsing

   void* operator new( size_t )  { return (void*) 1; }
   void operator delete( void* ) {}

public:
   ZipHeader() : timestamp(0), originalName(0) {}
   // ~ZipHeader() {}

	/**
	 *	Verifies that the header is valid
	 */
   E_RESULT parse( const buffer& inputdata ) UNCOMPRESSEDSECTION;

    /**
     *  May return the original name (after parsing, if present)
     */
   const char* getOriginalName() const { return originalName; }

    /**
     *  If parsing was succesful, this will return the start of data
     */
   const buffer& getData() const { return zipdata; }

    /**
     *  Returns the original size of the zipped data (unverified!)
     *  (reads the 32bit number at the *end* of all zipdata, it should have been in header)
     */
   size_t getUnzippedSize() const { return *((u32*) (zipdata.getEnd()-4)); }
};

}}	// namespace

#endif
