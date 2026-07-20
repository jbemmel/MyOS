#ifndef PAGEDIRECTORY_H
#define PAGEDIRECTORY_H

#include "PTE.h"

namespace MyOS {

namespace IO { class OStream; }

namespace MM {

class Page;

/// Represents a pagetable directory containing pagetable entries
class PageDirectory
{
    friend class IPhysicalMemoryImpl;
    friend class PTE;	// access to BASEADDRESS
    
protected:
   /// Constructor   @note clears all entries !!
   inline PageDirectory() {}
   PTE entries[ 1024 ];

   enum { BASEADDRESS = 0xFFC00000 };
public:	
    typedef u32 index_t;

    // could check i<=1023
    inline PTE& operator[]( index_t i ) { return entries[i]; }

    // Fixed mapping of linear address to PDT
    inline static PageDirectory& getDir( linadr_t forAddress )
    {
        return ((PageDirectory*)BASEADDRESS)[ getPBTIndex(forAddress) ];
    }

    /// Can be treated as a page, not using "operator Page*"
    Page* asPage() const { return (Page*) this; }

    s32 listTable( IO::OStream &buf ) const;

};
    
}}	// namespace

#endif
