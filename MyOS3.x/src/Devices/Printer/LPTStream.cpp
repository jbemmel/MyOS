#include "LPTStream.h"
#include "IO/IOUtil.h"
#include "Buffering/BufWriter.h"

namespace MyOS { namespace Devices { namespace LPT {

using namespace IO;

// virtual
s32
LPTStream::putBuffer( const buffer& buf )
{
   // PRINTF( "\nputBuffer %x size=%d firstchar=%c",
   //   buf.getData(), buf.getSize(), buf.getData()[0] );
   for ( u8* p = buf.getData(); p < buf.getEnd(); ++p ) {
      lpt1.write( *p );
   }
   return buf.getSize();
}

// virtual 
s32 
LPTStream::put( char c )
{
	lpt1.write( c );
	return 1;	
}

s32   // virtual
LPTStream::print( u64 number )
{
   char buffer[ 20 ];   // max. 16 digits + "0x", aligned (no '\0' needed)
   register char* p = buffer + sizeof(buffer);

   do {
      *--p = IO::IOUtil::toHexchar( number&0xF );
   } while (number>>=4);

   *--p = 'x';
   *--p = '0';
   size_t length = buffer + sizeof(buffer) - p;
   for ( u32 l=0;l<length;++l ) { lpt1.write( buffer[l] ); }
   return length;
}

// NOTE: Significant stack usage!
s32
LPTStream::printf( const char* format, ... )
{
   char bufspace[ 3*80 ];  // limited !
   BufWriter iobuf( buffer( (u8*) bufspace, sizeof(bufspace) ) );

   va_list pvar;
   va_start(pvar,format);
   /// output will be limited to the size of the buffer !
   s32 length = iobuf.va_snprintf( format, pvar );
   va_end(pvar);

   for ( s32 l=0;l<length;++l ) { lpt1.write( bufspace[l] ); }
   return length;
}

   
}}}   // namespace
