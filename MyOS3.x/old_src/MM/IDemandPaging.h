#ifndef IDemandPaging_H
#define IDemandPaging_H
#include "Core/IInterface.h"

using MyOS::Core::IInterface;

#include "memtypes.h"

namespace MyOS
{
namespace MM
{

namespace Exceptions {
    class PagingException {

    public:
        enum E_REASON {
            E_NULL_POINTER  = 0,
            E_INVALID_READ  = 1,
            E_INVALID_WRITE = 2,
        };

        inline PagingException( E_REASON r, linadr_t _eip, linadr_t lin, physadr_t phys ) throw()
            : reason(r), linear(lin), physical(phys), eip(_eip) {}

    private:
        E_REASON reason;
        linadr_t linear;
        physadr_t physical;
        linadr_t eip;
    };
}

namespace Paging
{

class DemandPagedEntity
{
public:
    /// Called when this entity must be fetched
    /**
     * This is called in the context of the page fault handler
     *
     * @return Physical page where loaded entity resides (gets mapped)
     */
    virtual physadr_t fetchNow() throw (Exceptions::PagingException) = 0;
};

/**
 *
 */
INTERFACE( IDemandPaging, "c5f5093d-07d2-4b5e-8b01-e77406bfa43e" )

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
};

}
}
} // namespaces
#endif
