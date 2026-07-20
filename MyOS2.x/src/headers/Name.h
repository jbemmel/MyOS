/***************************************************************************
                          Name.h  -  description
                             -------------------
    begin                : Thu Jul 25 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef NAME_H
#define NAME_H

#include "string.h" // strlen
#include "IO/IOUtil.h"

namespace MyOS {

/// A name consist of a pointer to characters and a length, to be able to construct names from read-only XML
typedef struct myos_name_t {
   const char* name;
   size_t length;

   myos_name_t() : name(""), length(0) {}
   myos_name_t( const char* s, size_t l ) : name(s), length(l) {}
   explicit myos_name_t( const char* s ) : name(s), length( s ? strlen(s) : 0) {}
   myos_name_t( const myos_name_t& o, size_t ofs=0 ) : name(o.name+ofs), length(o.length-ofs) {}

   bool startsWith( char c ) const { return length && name && name[0]==c; }
   operator const char*() const { return length ? name : ""; }

   char* strchr( char c ) const { char* p = MyOS::strchr(name,c); return p<name+length?p:0; }
   s32 indexOf( char c ) const { char* p = strchr(c); return p ? p-name : -1; }

   void copy( const myos_name_t& n ) {
      // NOTE! The buffer may get shortened here! length is altered
      strncpy( (char*) name, n.name, (length = n.length) );
   }

   void copy2ucase( const myos_name_t& n ) {
      
      // NOTE! The buffer may get shortened here! length is reduced
      length = n.length;      
      for (u32 c=0; c < length; ++c) {
         ((char*)name)[c] = IO::IOUtil::toUpper( n.name[c] );
      }
   }


   // returns substring starting at index (no check for valid index)
   myos_name_t substring( size_t index ) const { return myos_name_t( name+index, length-index ); }

   // returns substring from..to exclusive (to>=from, not checked)
   myos_name_t substring( size_t from, size_t to ) const {
      return myos_name_t( name+from, (to-from) );
   }

   bool equals( const myos_name_t &other ) const {
      return length == other.length && strncmp(name,other.name,length)==0;  
   }

} myos_name_t; /// @todo Perhaps use INamed instead (extended with length)

}  // namespace

#endif
