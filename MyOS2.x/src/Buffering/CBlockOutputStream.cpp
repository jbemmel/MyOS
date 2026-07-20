/***************************************************************************
                          CBlockOutputStream.cpp  -  description
                             -------------------
    begin                : Sat Aug 10 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/

#include "CBlockOutputStream.h"

// debug
//#include "Assertion.h"
//#include "Screen.h"

namespace MyOS {
namespace IO {

/// An outputstream class to write packets
CBlockOutputStream::CBlockOutputStream( size_t bls, buffer& blockbuffer, Observer& obs )
: writer( blockbuffer ), client(obs), blocksize(bls), bytesWritten(0)
{

}

bool CBlockOutputStream::checkCallback()
{
   size_t bytes = writer.getBytesWritten();
   // ASSERTIONv( bytes < 2*blocksize, Debug::AS_FATAL, bytes );
   if (bytes >= blocksize ) {
      buffer written = writer.getAsBuf();
      writer.setBuffer( client.onBytesWritten( buffer(written,0,blocksize) ));
      if (bytes > blocksize) {
         writer.putBuffer( buffer(written,blocksize), bytes-blocksize );
      }
      return true;
   }
   return false;
}

// IOStream
// virtual
bool
CBlockOutputStream::seek( s32 offset, bool isRelative )
{
   return -1;
}

// virtual
s32
CBlockOutputStream::getMarker() const
{
   // can be useful for counting #bytes added, but cannot be used for seek
   return bytesWritten;
}

// virtual
s32
CBlockOutputStream::flush()
{
   if (size_t bytes = writer.getBytesWritten()) {
      writer.setBuffer( client.onBytesWritten( writer.getAsBuf() ));
      return bytes;
   }
   return 0;
}

// virtual
myos_result_t
CBlockOutputStream::close()
{
   flush();
   return E_MYOS_SUCCESS;
}


// OStream
// virtual
s32
CBlockOutputStream::printf( const char* format, ... )
{
   va_list pvar;
   va_start(pvar,format);
   s32 result = writer.va_snprintf( format, pvar );   // assumption: fits
   va_end(pvar);

   if (result>0) bytesWritten += result;

   // assumption: max. 1 block
   checkCallback();
   return result;
}

// virtual
s32
CBlockOutputStream::print( u64 number )
{
   return -1;  // not yet
}

// virtual 
s32 
CBlockOutputStream::put( char c )
{
	writer.putByte(c);
	++bytesWritten;
	checkCallback();
	return 1;
}

// virtual
s32
CBlockOutputStream::putBuffer( const buffer& buf )
{
   s32 done=0;

   // first check if there is some partial content
   if (size_t bytes = writer.getBytesWritten()) {
      done = buf.getSize() < (blocksize-bytes) ? buf.getSize() : (blocksize-bytes);
      writer.putBuffer( buf, done );      // fill it out if possible
      bytesWritten += done;
      if (!checkCallback()) return done;  // not enough bytes to fill
   }

   // Always use caller provided buffer(!) no optimization possible here (?)
   while (done <= (s32) (buf.getSize()-blocksize)) {
      bytesWritten += blocksize;
      writer.putBuffer( buffer(buf,done), blocksize );
      writer.setBuffer( client.onBytesWritten( writer.getAsBuf() ));
      done += blocksize;
   }

   size_t left;
   if ((left = (buf.getSize()-done))) {    // put leftover bytes in iobuffer
      writer.putBuffer( buffer(buf,done,left), left );
      bytesWritten += left;
   }
   return done;
}

}} // namespace
