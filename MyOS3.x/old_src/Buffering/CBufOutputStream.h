/***************************************************************************
                          CBufOutputStream.h  -  description
                             -------------------
    begin                : Wed Aug 14 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef CBufOutputStream_H
#define CBufOutputStream_H

#include "IO/OStream.h"

namespace MyOS {
namespace IO {

/**
 * Outputstream that writes its bytes to a given array (up to a given limit)
 */
class CBufOutputStream : public OStream
{
  buffer buf;
  u8* curIndex;
public:
   inline CBufOutputStream( u8* data, size_t length ) : buf(data,length), curIndex(data) {}
   virtual ~CBufOutputStream() { close(); }
  
	/** @see IOStream#getBuffer */
	virtual const buffer getBuffer() const { return buf; }

   // IOStream
   virtual bool seek( s32 offset, bool isRelative );
   virtual s32 getMarker() const;
   virtual myos_result_t close();

   virtual s32 printf( const char* format, ... );
   virtual s32 print( u64 number );
   virtual s32 putBuffer( const buffer& buf );
   virtual s32 flush();
   virtual s32 put( char c );

   inline size_t getCount() const { return curIndex - buf.getData(); }

private:
   void operator delete( void* ) {}   // disallowed   
};

}}  // namespace

#endif
