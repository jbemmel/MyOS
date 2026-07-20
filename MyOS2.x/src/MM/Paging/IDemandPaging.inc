#ifdef INCLUDE_PHASE

#include "../memtypes.h"

namespace MyOS { 

namespace Exceptions { class PagingException {}; }

namespace MM { namespace Paging {

class DemandPagedEntity {
public:
   /// Called when this entity must be fetched
   /**
    * This is called in the context of the page fault handler
    * 
    * @return Physical page where loaded entity resides (gets mapped)
    */
   virtual physadr_t fetchNow() throw (Exceptions::PagingException) = 0;   
};

}}}

#else

/**
 * @param readonly: If true, writes cause page faults without calling fetchNow()
 *
 * NOTE: Could put 8-byte alignment requirement on entities, to have more bits
 *       to use for e.g. write-only
 */
virtual linadr_t map( Paging::DemandPagedEntity *first, size_t itemsize, size_t count, bool readonly ) = 0;

/**
 * Unmaps paged entities one by one, to be able to release physical bufs
 * @return mapped physical address, *including* paging bits, if any, or 0 if not mapped
 */
virtual physadr_t unmap( linadr_t page ) = 0;   

#endif
