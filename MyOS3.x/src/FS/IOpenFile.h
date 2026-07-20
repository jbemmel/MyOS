#ifndef IOPENFILE_H
#define IOPENFILE_H

#include "Core/IInterface.h"

namespace MyOS { namespace FS {

class FSException {};
class FileNotFoundException {};

/// Flags returned by read
enum E_READFLAGS {
   E_EOF = (1UL<<31),
};
   
/// Represents an open file
/**
 * NOTES:
 * - Files in MyOS are memory mapped (but you dont see this in the interface)
 * - 'rename' and 'delete' are file-meta operations and as such cannot be 
 *   performed on open files (although that could be implemented). They are 
 *   defined as FS operations instead
 * 
 */
class IOpenFile
{
public:
   // or perhaps use a non-virtual method + a data member in this class...
   virtual const myos_name_t& getName() const = 0;

   virtual size_t getSize() const = 0;

   /// To prevent paging delays at critical stages ?
   /// virtual void prefetch() = 0;

   /// Reads <count>(max 2^31-1) bytes from offset <offset> of this file
   /// (blocking, may suffer demand paging delays)
   /**
    * 
    * @param buf : Buffer to receive input, it is assumed that count <= buf.size
    * 
    * @return Number of bytes read, with E_EOF bit set when EOF is reached
    * 
    * @throw FSException If offset >= filesize
    */
   virtual size_t read( u32 offset, size_t count, const buffer& buf ) const 
      throw (FSException) = 0;

   virtual void write( u32 offset, size_t count, const buffer& buf ) 
      throw (FSException) = 0;

   virtual void close() = 0;

   // 'getMemoryPtr' for memory mapped fast access (dangerous: reading beyond EOF)
   // @return 0 if not supported, else valid memory-mapped pointer to data
   virtual void* getMemoryPtr() const = 0;
};   

class IFileAsyncCompletion;
class AsyncHandle;

/// Same as IOpenFile, but supports asynchronous operations using events
class IOpenFileAsync
{
public:
   // or perhaps use a non-virtual method + a data member in this class...
   virtual const myos_name_t& getName() const = 0;

   virtual size_t getSize() const = 0;

   virtual const AsyncHandle& readAsync( u32 offset, size_t count, IFileAsyncCompletion& cb ) const 
      throw (FSException) = 0;
      
   virtual void close() = 0;   
};

class IFileAsyncCompletion
{
public:   
   virtual void onReadDone( AsyncHandle& h ) = 0;
   virtual void onError( AsyncHandle& h ) = 0;      
};

/// Represents an asynchronous operation in progress
class AsyncHandle 
{
   const IOpenFileAsync& file;   // file on which operation is pending
   u32 offset;
   size_t count;

   const buffer* buf;

public:
   // Used for initial request, count maintains last offset
   AsyncHandle( const IOpenFileAsync& f, u32 o, size_t c )
      : file(f), offset(o), count(o+c) {}

   // reuses part of existing handle, when next buffer is received
   AsyncHandle( AsyncHandle& h, const buffer& b )
      : file(h.file), offset(h.offset), count(b.getSize()), buf(&b)
   {
      // adjust the offset in the original handle, check for EOF
      h.offset += count;
      if (h.offset >= h.count) {
         count -= (h.offset - h.count);   // adjust count
         h.offset = h.count;              // mark it done
      }
   }      

   // isLast for partial requests, see above code
   inline bool isLastPartial() const { return offset==count; }

   /// Cancels this operation
   void cancel();

   // Assumes buffer valid, will compensate for EOF
   inline const buffer getBuffer() const { 
      return buffer( *buf, 0, count );
   }

   // isLast for when the entire file is read
   inline bool isLast() const { return offset+count >= file.getSize(); }

   inline u32 getOffset() const { return offset; }
   inline const IOpenFileAsync& getFile() const { return file; }

};
 
}} // namespace

#endif
