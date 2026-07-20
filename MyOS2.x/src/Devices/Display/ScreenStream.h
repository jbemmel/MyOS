#ifndef SCREENSTREAM_H
#define SCREENSTREAM_H

#include "IO/OStream.h"
#include "ScreenBuffer.h"

namespace MyOS { namespace Devices { namespace Display {
   
/// An outputstream class to write to the 80x24 textscreen
class ScreenStream : public IO::OStream
{
public:
    /// @todo Work this out
    ScreenStream( ScreenBuffer::Cursor& cursor, bool buffered );

   // IOStream
   virtual bool seek( s32 offset, bool isRelative );
   virtual s32 getMarker() const { return cursor.getOffset(); }
   virtual myos_result_t close() { return E_MYOS_E_NOTIMPL; }
   virtual s32 flush() { return -1; }

    /** @todo: implement proper formatting here */
    virtual s32 printf( const char* format, ... );
    virtual s32 print( u64 number );
    virtual s32 putBuffer( const buffer& buf );
    virtual s32 put( char c );

private:
    // char* cursor;    // for buffered

    ScreenBuffer::Cursor& cursor;      // direct
};

}}} // namespace

#endif
