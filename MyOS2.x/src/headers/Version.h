/***************************************************************************
                          version.h  -  description
                             -------------------
    begin                : Sat Aug 18 2001
    copyright            : (C) 2001 by jeroen
    email                : jeroen@thebem
 ***************************************************************************/
#ifndef MYVERSION_H
#define MYVERSION_H

#include "types.h"

// namespace ??

/**
 * Structure to hold version information (major.minor)M
 * minor versions indicate up/downward compatibility
 */
struct VERSION {

    /// Stored in little endian(!), so use (ma<<16|mi) to construct from 2 u16's
    u16 minor;
    u16 major;

   inline VERSION& operator=( u32 i ) {
      *((u32*)this)=i;
      return *this;
   }

   inline VERSION& operator=( const VERSION& v ) {
      return (*this = *((u32*)&v));
   }

   inline VERSION( u16 ma, u16 mi )  { *this = (ma<<16|mi); }
   inline VERSION( const VERSION& v )  { *this = *((u32*)&v); }

   // to use like 'header.version' in constructor call
   explicit inline VERSION( u32 mami )        { *this = mami; }

   // used in IComponent
   VERSION() { *this = 0; }

   /// conversion to u32
   inline operator u32() const { return *((u32*) this); }

    /// Returns a version representing a wildcard (any)
    static inline VERSION ANY()
    {
        return VERSION(0,0);
    }

    /// Compares 2 versions for equality, both major and minor must be equal
    inline bool operator ==( const VERSION v ) const
    {
        return ((major==v.major) && (minor==v.minor))
         || ((u32)v==0) || ((u32)*this==0); // prevent recursion, no ANY()
    }

    // Checks if this object exactly matches 'ANY'
    inline bool isAny() const {
      return ((u32)*this)==0;
    }

    inline bool operator !=( const VERSION v ) const
    {
        return ! ( *this == v );
    }

    /// Compares 2 versions for less than
    inline bool operator <( const VERSION v ) const
    {
        return (*this != v) && ((major<v.major)  ||
               ((major==v.major) && (minor<v.minor)));
    }

    inline bool operator >( const VERSION v ) const
    {
        return (*this != v) && ((major>v.major)  ||
               ((major==v.major) && (minor>v.minor)));
    }

    inline bool operator >=( const VERSION v ) const
    {
         return (major>v.major) || ((major==v.major) && (minor>=v.minor))
            || ((u32)v==0) || ((u32)*this==0);  // prevent recursion, no ANY()
    }

    /**
     *  A version is compatible with v if the major version matches
     *  and the minor version is at least as high as v.minor
     *
     *  @note 'ANY' is compatible with all versions
     */
    inline bool compatibleWith( const VERSION v ) const
    {
        return (*this==v) || (((major==v.major) && (minor >= v.minor)));
    }
};

#include "IO/OStream.h"

inline MyOS::IO::OStream& operator<<( MyOS::IO::OStream& o, const VERSION v )
{
   o.printf( "%d.%d", v.major, v.minor );
   return o;
}

#endif
