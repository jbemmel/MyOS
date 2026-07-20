/***************************************************************************
                          UUID.h  -  description
                             -------------------
    begin                : Sat May 4 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef UUID_H
#define UUID_H

#include "string.h" // memcmp

namespace MyOS {

// Actually, UUID can be considered a special kind of name...
class UUID
{
   char uuid[36];
public:
   explicit UUID( const char* const str ) {
      strncpy( uuid, str, sizeof(uuid) );
   }

   UUID( char str[37] /* with terminating 0 */ ) {
      strncpy( uuid, str, sizeof(uuid) );
   }

   /// Compares two uuid's for equality
   bool operator ==( const UUID& other ) const {
      return memcmp( uuid, other.uuid, sizeof(uuid) )==0;
   }

   /// Allows to compare an embedded uuid string to this object
   bool operator ==( const char* other ) const {
      return memcmp( uuid, other, sizeof(uuid) )==0;
   }


   bool operator !=( const UUID& other ) const {
      return !(*this==other);
   }

   /**
    * @note Not zero-terminated!
	*/
   const char* asString() const { return uuid; }
};

// Include zero-terminating '0' character
typedef const char uuid_t[37];

}  // namespace

#endif
