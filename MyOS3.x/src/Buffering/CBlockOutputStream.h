/***************************************************************************
                          CBlockOutputStream.h  -  description
                             -------------------
    begin                : Sat Aug 10 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef CBLOCKOUTPUTSTREAM_H
#define CBLOCKOUTPUTSTREAM_H

#include "IO/OStream.h"
#include "BufWriter.h"

namespace MyOS {
namespace IO {

/// An outputstream class to write packets
class CBlockOutputStream : public OStream
{
public:
   /// Offers an OStream with callbacks after each N bytes
   /**
    * To make the implementation efficient & less complicated, it is assumed that N is larger than
    * the typical printf() #bytes. The blockbuffer should be slightly larger than N to overcome
    * buffer overflow problems (it's hard to do 'half' printfs)
    */
   class Observer;
   CBlockOutputStream( size_t blocksize, buffer& blockbuffer, Observer& obs );

   virtual ~CBlockOutputStream() { close(); }

   // IOStream
   virtual s32 flush();
   virtual myos_result_t close();
   virtual bool seek( s32 offset, bool isRelative );
   virtual s32 getMarker() const;

   // OStream

   /**
    * It is assumed that all printfs 'fit' in the blockbuffer
    */
   virtual s32 printf( const char* format, ... );
   virtual s32 print( u64 number );
   virtual s32 putBuffer( const buffer& buf );
   virtual s32 put( char c );   

   class Observer {
   public:
      // The 'bytes' parameter is guaranteed to be (part of) the buffer provided by the client
      virtual buffer onBytesWritten( const buffer& bytes ) = 0;
   };

private:
   bool checkCallback();

   // not allowed for now
   inline void operator delete( void* ) {}

   BufWriter writer;
   Observer& client;
   size_t blocksize;
   size_t bytesWritten; // for getMarker
};

}} // namespace

#endif
