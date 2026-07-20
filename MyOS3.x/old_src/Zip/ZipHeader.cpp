#include "ZipHeader.h"
#include "string.h"

namespace MyOS {
namespace Zip {

enum E_GZIPFLAGS {
    E_ASCII_FLAG    = 0x01, /* bit 0 set: file probably ASCII text */
    E_CONTINUATION  = 0x02, /* bit 1 set: continuation of multi-part gzip file */
    E_EXTRA_FIELD   = 0x04, /* bit 2 set: extra field present */
    E_ORIG_NAME     = 0x08, /* bit 3 set: original file name present */
    E_COMMENT       = 0x10, /* bit 4 set: file comment present */
    E_ENCRYPTED     = 0x20, /* bit 5 set: file is encrypted */
    E_RESERVED      = 0xC0, /* bit 6,7:   reserved */
};

/// The fixed part of the zipheader structure
struct ZIPHEADER
{
    u8 magic0, magic1;
    u8 method;
    u8 flags;
    u32 timestamp;
    u8 extraFlags;
    u8 ostype;

    // Here the flags-depending part starts
    u8 optionaldata;
} PACKED;

E_RESULT
ZipHeader::parse( const buffer& inputdata )
{
    /// First check if there is enough data
    if (inputdata.getSize()<40) return E_BAD_HEADER;

    ZIPHEADER *header = (ZIPHEADER*) inputdata.getData();

    // check magic number
    if ((header->magic0!=037) || ((header->magic1!=0213 && header->magic1!=0236))) {
      return E_BAD_MAGIC_NUMBER;
    }

    // check compression method #8 (deflated)
    if (header->method != 8) {
        return E_INVALID_COMPRESSION_METHOD;
    }

    // Check for unsupported input flags
    u8 flags = header->flags;
    if ((flags & E_ENCRYPTED) != 0) {
       return E_ENCRYPTED_INPUT;
    }
    if ((flags & E_CONTINUATION) != 0) {
       return E_MULTIPART_INPUT;
    }
    if ((flags & E_RESERVED) != 0) {
       return E_INVALID_FLAGS;
    }

    // Get timestamp (little endian)
    timestamp = header->timestamp;

    /*** START OF DYNAMIC PART **/
    u8* data = &header->optionaldata;

    // Skip extra fields (little endian)
    if ((flags & E_EXTRA_FIELD) != 0) {
       u16 len = *((u16*) data); data+=2;
       data += len;
    }

    // Get original file name if it was truncated
    if ((flags & E_ORIG_NAME) != 0) {
        originalName = (char*) data;
       // Discard the old name
        data += strlen((const char*)data)+1;
    }

    // Discard file comment if any
    if ((flags & E_COMMENT) != 0) {
       data += strlen((const char*)data)+1;
    }

    // keep start of data
    zipdata = buffer( data, inputdata.getSize() - (data - inputdata.getData()) );
    return E_OK;
}

}} // namespace
