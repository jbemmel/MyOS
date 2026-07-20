/***************************************************************************
                          NVPAIR.h  -  description
                             -------------------
    begin                : Sun Mar 10 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef NVPAIR_H
#define NVPAIR_H

#include "string.h"
#include "IOUtil.h"

namespace MyOS {

/**
 *	Struct representing a name-value pair
 */
class NVPAIR {
   typedef const char* string_t;

   /// Zero-terminated string representing the name, != 0
   string_t name;
	
   /// Zero-terminated string representing the value, can be null
   const void* value;

public:
   // Converts '0' into "" for the name
   inline NVPAIR( string_t n, const void* v ) : name( n ? n : ""), value(v) {}

   // needed for declaring array on stack (?)
   inline NVPAIR() : name(""), value(0) {}
   inline NVPAIR( const NVPAIR& n ) : name(n.name), value(n.value) {}

   inline NVPAIR& operator =( const NVPAIR& n ) {
      this->name = n.name;
      this->value = n.value;         
      return *this; 
   }

   inline string_t getName() const { return name; }
   inline const void* getValue() const { return value; }   
   inline string_t asString() const { return (string_t) value; }

   template <typename T>
   inline T* getAs() { return (T*) value; }

   // Need write access for parsing
   inline void setName( string_t n ) { this->name = n; }
   inline void setStringValue( string_t val ) { this->value = (void*) val; }

   inline bool hasName( string_t n ) { return strcmp(name,n)==0; }
   inline s32 asSigned() const { return value ? IO::IOUtil::atoi_signed( (string_t) value) : 0; }
   inline u32 asUnsigned() const { return value ? IO::IOUtil::atoi( (string_t) value) : 0; }
   // u32 asHex() const { return value ? IO::IOUtil::atoi_hex(value) : 0; }
   
   // to zero-terminate an array, use an invalid NVPAIR last
   inline bool isValid() const { return name!=0; }
};


}	// namespace

#endif
