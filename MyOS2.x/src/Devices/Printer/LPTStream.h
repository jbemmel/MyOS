#ifndef LPTSTREAM_H
#define LPTSTREAM_H

#include "IO/OStream.h"
#include "LPTPort.h"

namespace MyOS { namespace Devices { namespace LPT {

// Simple class, useful for debugging   
class LPTStream : public IO::OStream {

public:
   // virtual ~LPTStream() { close(); }

   // IOStream
   virtual myos_result_t close() { return E_MYOS_SUCCESS; }
   virtual s32 getMarker() const { return -1; }
   virtual bool seek( s32 offset, bool isRelative ) { return false; }
   
   // OStream
   virtual s32 printf( const char* format, ... );
   virtual s32 print( u64 number );
   virtual s32 putBuffer( const buffer& buf );
   virtual s32 put( char c );
   virtual s32 flush() { return 0; }
   
private:
   LPTPort lpt1;   
};   
   
}}}   // namespace

#endif
