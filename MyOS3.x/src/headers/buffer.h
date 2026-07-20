#ifndef BUFFER_H
#define BUFFER_H

#include "types.h"
#include "string.h"     // strlen

namespace MyOS {

/// A very basic class that holds a pointer to storage and the size of that storage
class buffer
{
   size_t size;   /// The number of bytes data points to
   u8* data;      /// Pointer to buffer storage, <code> null </code> for an invalid buffer

   // prevent dynamic allocation
   inline void operator delete( void* ) {}

public:
   buffer() : size(0), data(0) {}
   
   /// Creates a buffer from a 0-terminated string
   inline explicit buffer( const char* zstr ) : size( strlen(zstr) ), data( (u8*) zstr) {}
   
   inline buffer( u8* d, size_t s ) : size(s), data(d) {}

   // copy constructor, can be partial
   inline buffer( const buffer& other, u32 ofs=0 )
        : size(other.size-ofs), data(other.data + ofs) {}

   // copy constructor, partial and smaller
   inline buffer( const buffer& other, u32 ofs, u32 s )
        : size(s), data(other.data + ofs) {}


   // destructor, invalidates the buffer
   // ~buffer() { size=0; data=0; }

   inline size_t getSize() const { return size; }
   inline u8* getData() const    { return data; }
   inline bool isValid() const   { return data && size>0; }
   inline u8* getEnd() const     { return data+size; }

   inline u8& operator[]( size_t index ) { return data[index]; }   // no check
   inline const u8& operator[]( size_t index ) const { return data[index]; }

   // GetData cast to particular type
   template <typename T>
   inline T& getDataAs() const { return *((T*)data); }
};

}	// namespace

#endif
