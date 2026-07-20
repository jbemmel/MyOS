/*
    Memory mapping is confusing!

    A logical reasoning might seem like this: There is 1 PBT which maps 1024 PDTs, so a total of 1025 which does not fit into 4Mb

    However:
    - the PBT contains all the physical addresses of the PDTs
    - PDT[1023] also contains all the physical addresses of the PDTs

    Therefore, the PBT == PDT[1023] !!

*/

#include "PBT.h"
#include "Page.h"

#include "MM/MMComponent.h"   // for allocating spare page
#define PHYS MMComponent::getInstance().impl1

namespace MyOS {
namespace MM
{

/**
 *  Constructor, copies appropriate entries from current PBT
 */
PBT::PBT( physadr_t mapping )
{
   init( mapping );
}

void
PBT::init( physadr_t mapping )
{
	// Copy appropriate PBT entries from the (current) parent process
	entries[ 0 ]    = getCurrentPBT()[0];	     // first 4MB, shared
	// entries[ 1021 ] = getCurrentPBT()[1021];	 // physical page pools @ 0xff400000

	// kernel page mapping 0xFF800000 - 0xFFC00000 including the PBT, shared
	entries[ 1022 ] = getCurrentPBT()[1022];

	// pagetable directories, mostly private
	entries[ 1023 ].initialize( mapping, E_PAGE_KERNEL );
}

/**
 *  Returns a reference to the page directory for a given linear address,
 *  mapping it atomically if needed
 *
 * @todo Could also do this on demand, in pagefault handler
 */
PageDirectory&
PBT::getDirectory( linadr_t forAddress )
{
    // always keep a spare page in advance, to map a directory if needed
    static physadr_t spare = PHYS.allocateFrame();

    // avoid RACE condition of 2 threads calling getDirectory in parallel, both using same 'spare'
    // (alternative: per-thread spare, wasteful)
    physadr_t spareCopy = ((atomic32*) &spare)->Replace( 0 );
    if ( RARELY( spareCopy==0 ) ) {
        spareCopy = PHYS.allocateFrame();
        if ( !( getCurrentPBT().entries[ getPBTIndex(forAddress) ].mapifnull(spareCopy, E_PAGE_KERNEL) ) ) {
            PHYS.releaseFrame( spareCopy );
        }
    } else {
        // non-blocking check to see if already mapped
        if ( RARELY( getCurrentPBT().entries[ getPBTIndex(forAddress) ].mapifnull(spareCopy, E_PAGE_KERNEL) ) )
        {
            // (rarely) allocate a new page for next time
            spare = PHYS.allocateFrame();
        } else {
            spare = spareCopy;  // restore it
        }
    }
    return getDir( forAddress );
}

/*
bool
PBT::isMapped( const PageDirectory& dir )
{
	return getCurrentPBT().entries[ getPDTIndex(&dir) ].isMapped();
}

bool
PBT::getMapping( const PageDirectory& dir )
{
  return getCurrentPBT().entries[ getPDTIndex(&dir) ].getMapping();
}
*/

}}   // namespace

