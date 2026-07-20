/***************************************************************************
                          CBufInputStream.h  -  description
                             -------------------
    begin                : Wed Aug 14 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef CBUFINPUTSTREAM_H
#define CBUFINPUTSTREAM_H

#include "IStream.h"
#include "string.h" // strlen

namespace MyOS {
namespace IO {

/**
 * Inputstream that reads its bytes from a given array (up to a given limit)
 */
class CBufInputStream : public IStream
{
  buffer buf;
  u8* curIndex;

public:
   CBufInputStream( u8* data, size_t length )
      : buf(data,length), curIndex(data) {}
   
   explicit CBufInputStream( const char* s )
      : buf( (u8*) s,strlen(s)), curIndex( (u8*) s) {}

   virtual ~CBufInputStream() { close(); }
  
  	/** @see IOStream#read */
	virtual myos_result_t read( buffer& toBuffer, flags_t options );

	/** @see IOStream#getBuffer */
	virtual const buffer getBuffer() const;

   // IOStream
   virtual bool seek( s32 offset, bool isRelative );
   virtual s32 getMarker() const;
   virtual bool setDelimiter( char delim ) { return false; }   // could support
   virtual myos_result_t close();
   
private:
   void operator delete( void* ) {}   // disallowed   
};

}}  // namespace

#endif
