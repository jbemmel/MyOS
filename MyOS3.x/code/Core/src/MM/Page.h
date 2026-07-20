/***************************************************************************
                          Page.h  -  description
                             -------------------
    begin                : Sat Apr 27 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/

#ifndef PAGE_H
#define PAGE_H

#include "memtypes.h"   // PTE attributes
#include "string.h"           // memset_aligned

namespace MyOS {
namespace MM {

/// Represents a single page of linear/physical memory
/**
 *@author Jeroen van Bemmel
 */
class Page {
   u32 dwords[ 1024 ];

   // todo I could implement these
   void* operator new( size_t, pteflags_t atts = E_PAGE_KERNEL );
   inline void operator delete( void* ) {}

public:
   /// Can be called by typecasting any linadr_t to a Page
   physadr_t getMapping() const;

   pteflags_t getAttributes() const;
   void setAttributes( pteflags_t attributes );

   // Verify that this page is cleared (should be all zeroes)
   bool isZero() const {
      for (u32 i=0; i<1024; ++i) if (dwords[i]) return false;
      return true;
   }

   inline void clear() {
      memset_aligned( this, 0, sizeof(Page) );
   }
};

// Debug util, use with address param
#define ASSERT_MAPPED(p) \
   ASSERTIONv( ((Page*)p)->getAttributes() & E_PAGE_PRESENT, Debug::AS_FATAL, (p) );
#define ASSERT_WRITABLE(p) \
   ASSERTIONv( ((Page*)p)->getAttributes() & E_PAGE_READWRITE, Debug::AS_FATAL, (p) );

}} // namespace

#endif
