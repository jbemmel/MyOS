/***************************************************************************
                          CBufInputStream.cpp  -  description
                             -------------------
    begin                : Wed Aug 14 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#include "CBufInputStream.h"
#include "string.h"

// #include "Screen.h"

namespace MyOS {
namespace IO {

// virtual 
myos_result_t 
CBufInputStream::read( buffer& toBuffer, flags_t options )
{
   size_t tocopy = (size_t) min( (buf.getEnd()-curIndex), toBuffer.getSize() );
   memcpy( toBuffer.getData(), curIndex, tocopy );
   curIndex += tocopy;
   return (myos_result_t) tocopy;
}

// virtual 
const buffer 
CBufInputStream::getBuffer() const
{
   return buf;   // whole buffer ?
}

// virtual 
bool 
CBufInputStream::seek( s32 offset, bool isRelative )
{
   u8* index = (isRelative ? curIndex : buf.getData())+offset;
   if (index>=buf.getData() && index<buf.getEnd()) {
      curIndex=index;
      return true;      
   }
   return false;
}

// virtual 
s32 
CBufInputStream::getMarker() const
{
   return curIndex-buf.getData();   
}

// virtual 
myos_result_t 
CBufInputStream::close()
{
   curIndex=buf.getEnd();
   return E_MYOS_SUCCESS;   
}

}}  // namespace
