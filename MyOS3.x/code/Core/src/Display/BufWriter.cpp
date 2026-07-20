/***************************************************************************
                          BufWriter.cpp  -  description
                             -------------------
    begin                : Wed Apr 10 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#include "BufWriter.h"
#include "IOStream.h"      // for PRINTF_MAX_FORMAT_LENGTH
#include "IOUtil.h"
#include "string.h"
#include "stdarg.h"
#include "Name.h"

// #include "Assertion.h"
#define ASSERTION(A,S)
#define ASSERTIONv(A,S,v)

namespace MyOS {
namespace IO {

BufWriter::BufWriter( const buffer& buf )
   : obuf(buf), index( (char*) buf.getData() )
{

}

s32
BufWriter::va_snprintf( const char* fmt, va_list pvar)
{   
   char d[10];
   u32 u;
   s32 l = getSpace();

   if(!fmt || (l < 1)) return 0;

   char* b = (char*) index;
   const char* const _start = b;
   ++l;	// so loop can use pre-decrement

   char f;
   while( (--l>0) && (f=*fmt)) {
      if(f != '%'){	
         *b++ = f;
      } else {
         f = *++fmt;
         switch(f){
            case 's': // string
            {
               const char *t = va_arg(pvar,char *);
               if (!t) t = "<null>";         // make sure t points somewhere
               size_t length = strlen(t);
               ASSERTIONv( length <= IOStream::PRINTF_MAX_FORMAT_LENGTH, Debug::AS_FATAL, length );
               if (( l-= (length-1) ) < 0) {
			      goto bufferfull;
			   }
			   memcpy( b, t, length );
   			   b += length;
               break;
            }

			case 'S': // Unicode string, ugh, not efficient, not correct
			{
				u16* uc = va_arg(pvar,u16*);	// no NULL check...
				u16 ch;
				while ( (ch = *uc++) ) {
					if (--l<0) goto bufferfull;
					*b++ = (char) ch;
				}							
				break;
			}

            case 'c': // single character, "is promoted to int" says GCC
                *b++ = (char) va_arg(pvar,int);
                break;

            case 'b':	// JVB: bool, promoted to int
				*b++ = va_arg(pvar,int) ? '1' : '0';
				break;							

            case 'n':   // JVB: pointer to myos_name_t
            {
               myos_name_t *name = va_arg(pvar,myos_name_t*);
               if ((l -= name->length)<0) goto bufferfull;
               memcpy( b, name->name, name->length );
               b += name->length;
               break;
            }

			case 'r':	// JVB: repeat next character <arg> times
			   u = va_arg(pvar,u32);
               ASSERTIONv( u <= IOStream::PRINTF_MAX_FORMAT_LENGTH, Debug::AS_FATAL, u );
               if (u>(u32)l) u=l;
					f = *++fmt;
               l -= u;
               memset( b, f, u );
               b += u;
					break;

            case 'x': // 8 digit, unsigned 32bit hex integer
			{
               if(l < 7) {		// 1 already subtracted
                     goto bufferfull;
               }
               u = va_arg(pvar,unsigned int);
               for(s32 i=7; i>=0; i--){
                  b[i] = IOUtil::toHexchar( u & 0x0F );
                  u >>= 4;
               }
               b+=8;
               l-=7;
               break;
			}

            case 'd': // signed integer
			{
                s32 n = va_arg(pvar,s32);
                if(n < 0) {
                    u = -n;
                    *b++ = '-';
                    if(!(--l)) goto bufferfull;
                } else {
                    u = n;
                }
                goto u2;
			}

            case 'u': // unsigned integer
			{
                u = va_arg(pvar,unsigned int);
              u2:
                s32 i = 9;
                do {
                    d[i] = (u % 10) + '0';
                    u /= 10;
                    i--;
                } while(u && i >= 0);
                while(++i < 10){
                    *b++ = d[i];
                    if(!(--l)) goto bufferfull;
                }
                break;
			}

            case 'X': // 2 digit, unsigned 8bit hex int
				{
                if(l < 2) goto bufferfull;
                s32 n = va_arg(pvar,s32);
                *b++ = IOUtil::toHexchar( (n & 0xF0) >> 4 );
                *b++ = IOUtil::toHexchar( n & 0x0F );
                --l;
                break;
				}

            default:
                *b++ = f;
            }
        }
        fmt++;
    }
	// *b = 0;	dont terminate
	if (l>=0) return ((index=b) - _start);
bufferfull:
	return -(((index=b)-_start));	
}

size_t
BufWriter::putBuffer( const buffer& buf, size_t amount )
{
   size_t tocopy = min( amount, getSpace() );
   memcpy( index, buf.getData(), tocopy );
   index += tocopy;
   return tocopy;
}

}} // namespace
