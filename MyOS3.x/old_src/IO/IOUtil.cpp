/***************************************************************************
                          IOUtil.cpp  -  description
                             -------------------
    begin                : Wed Apr 10 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#include "IOUtil.h"

namespace MyOS {
namespace IO {

const char IOUtil::hexdigits[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

/**
 * My own addition, compares 2 strings but sorts numbered items differently
 * so { string2, string10 } will be in the proper order
 *
 * @param s1, s2  - Strings to compare
 * @return 0 if s1==s2, <0 if s1<s2, >0 otherwise
 */
int
IOUtil::strncmpa( const char* s1, const char* s2, size_t maxlength )
{
	while ( maxlength && *s1 /* && *s2 */ && (*s1==*s2) ) { ++s1; ++s2; --maxlength; }
	if (maxlength && isDigit(*s1) && isDigit(*s2)) return atoi(s1) - atoi(s2);
	else return maxlength ? (*s1 - *s2) : 0;
}

u32
IOUtil::atoi( const char* n )
{
   register u32 r=0;
   register char c;
   while (isDigit(c = *n++)) { r = 10*r + (c-'0'); }
   return r;
}

u32
IOUtil::atoi_hex( char*& n )
{
   register u32 r=0;
   register char c;
   while (isHexDigit(c = *n++)) {
      r = 16*r + toU8(c);
   }
   return r;
}

}} // namespace
