/***************************************************************************
                          IOUtil.h  -  description
                             -------------------
    begin                : Wed Apr 10 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef IOUTIL_H
#define IOUTIL_H

#include "types.h"
#include "string.h" // strlen

namespace MyOS {
namespace IO {

/// Utility class for common helper functions @todo These are not always related to IO -> move ?
class IOUtil
{
   /// For conversion of digit to ASCII character
   static const char hexdigits[16];

public:
   inline static char toHexchar( u8 hexdigit )  { return hexdigits[hexdigit]; }
   inline static u8 toU8( char hexdigit );

   inline static bool isUpper( char c )      { return (c>='A'&&c<='Z'); }
   inline static bool isLower( char c )      { return (c>='a'&&c<='z'); }
   inline static bool isDigit( char c )      { return (c>='0'&&c<='9'); }
   inline static bool isAlpha( char c )      { return isLower(c)||isUpper(c); }
   inline static bool isHexDigit( char c )   { return isDigit(c) || (c>='a'&&c<='f') || (c>='A'&&c<='F'); }

   inline static char toUpper( char c ) { return isLower(c) ? (c - ('a'-'A')) : c; }

   /// Interprets a string of '0'..'9' as unsigned number (until another character is found)
   static u32 atoi( const char* n );

   /// Signed version of atoi, @todo change atoi to return signed
   static s32 atoi_signed( const char* n );

   /// hex version of atoi, advances p, @todo: merge
   static u32 atoi_hex( char*& p );

   /// Compares two strings (like strncmp) for at most maxlength chars 
   /// but also sorts on ASCII numbers in those strings
   static int strncmpa( const char* s1, const char* s2, size_t maxlength );

   /// Compares to strings (like strcmp) but also sorts on ASCII numbers in those strings
   inline static int strcmpa( const char* s1, const char* s2 ) {
      return strncmpa( s1, s2, strlen(s1) );
   }

private:
   IOUtil();
};

inline s32
IOUtil::atoi_signed( const char* n )
{
   return n[0]=='-' ? - (s32) atoi(++n) : atoi(n);
}

inline u8
IOUtil::toU8( char d )
{
   if (d>='0' && d<='9') return d-'0';
   else if (d>='a' && d<='f') return d-'a'+10;
   else if (d>='A' && d<='F') return d-'A'+10;
   else return 0;
}


}} // namespace

#endif
