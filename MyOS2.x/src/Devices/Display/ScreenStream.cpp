/***************************************************************************
                          ScreenStream.cpp  -  description
                             -------------------
    begin                : Mon Apr 8 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#include "MyOSConfig.h"
#ifdef CONFIG_IDisplay

#include "ScreenStream.h"
#include "string.h"
#include "Buffering/BufWriter.h"
#include "IO/IOUtil.h"

#include "stdarg.h"

#include "debug.h"

namespace MyOS { namespace Devices { namespace Display {
   
ScreenStream::ScreenStream( ScreenBuffer::Cursor& _cursor, bool buffered )
   : OStream(), cursor( _cursor )
{

}

bool  // virtual @see IOStream#seek
ScreenStream::seek( s32 offset, bool isRelative )
{
   return isRelative ? cursor.setPositionRelative(offset) : cursor.setPosition(offset);
}

// virtual @see OStream#printf ( @todo also support color )
s32
ScreenStream::printf( const char* format, ... )
{
   char bufspace[ 3*80 ];  // limited !
   IO::BufWriter iobuf( buffer( (u8*) bufspace, sizeof(bufspace) ) );

   va_list pvar;
   va_start(pvar,format);
   /// output will be limited to the size of the buffer !
   s32 length = iobuf.va_snprintf( format, pvar );
   
   // Duplicate on printer port? No, segfaults (probably eh due to destructor)
   // DPRINTK( "%n", &myos_name_t(bufspace,length) );
   
   va_end(pvar);

   cursor.print( bufspace, length>=0 ? length : sizeof(bufspace), E_DEFAULT );
   return length;
}


s32   // virtual
ScreenStream::print( u64 number )
{
   char buffer[ 20 ];   // max. 16 digits + "0x", aligned (no '\0' needed)
   register char* p = buffer + sizeof(buffer);

   do {
      *--p = IO::IOUtil::toHexchar( number&0xF );
   } while (number>>=4);

   *--p = 'x';
   *--p = '0';
   size_t length = buffer + sizeof(buffer) - p;
   cursor.print( p, length, E_DEFAULT );
   return length;
}

s32
ScreenStream::putBuffer( const buffer& buf )
{
   // PRINTF( "\nputBuffer %x size=%d firstchar=%c",
   //   buf.getData(), buf.getSize(), buf.getData()[0] );
   cursor.print( (const char*) buf.getData(), buf.getSize(), E_DEFAULT );
   return buf.getSize();
}

// virtual 
s32 
ScreenStream::put( char c )
{
	cursor.print( c, E_DEFAULT );	
	return 1;
}

}}} // namespace

#endif
