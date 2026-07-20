/***************************************************************************
                          CBufOutputStream.cpp  -  description
                             -------------------
    begin                : Fri Aug 23 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#include "CBufOutputStream.h"
#include "BufWriter.h"

namespace MyOS {
namespace IO {

// virtual
bool
CBufOutputStream::seek( s32 offset, bool isRelative )
{
   u8* newIndex = offset + (isRelative ? curIndex : buf.getData());
   if (newIndex >= buf.getData() && newIndex < buf.getEnd()) {
      curIndex = newIndex;
      return true;
   } else return false;
}

// virtual
s32
CBufOutputStream::getMarker() const
{
   return curIndex-buf.getData();
}

// virtual
myos_result_t
CBufOutputStream::close()
{
   curIndex = buf.getEnd();
   return E_MYOS_SUCCESS;
}

// virtual
s32
CBufOutputStream::printf( const char* format, ... )
{
   BufWriter writer( buffer(curIndex,buf.getEnd()-curIndex) );
   va_list pvar;
   va_start(pvar,format);
   s32 result = writer.va_snprintf( format, pvar );   // assumption: fits
   va_end(pvar);
   return result;
}

// virtual
s32
CBufOutputStream::print( u64 number )
{
   return -1;
}

// virtual
s32
CBufOutputStream::putBuffer( const buffer& data )
{
   size_t tocopy = data.getSize() <? (u32) (buf.getEnd()-curIndex);
   memcpy( curIndex, data.getData(), tocopy );
   curIndex += tocopy;
   return tocopy;
}

// virtual
s32
CBufOutputStream::flush()
{
   return 0;   // ?? what else
}

}}  // namespace
