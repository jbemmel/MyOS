#include "PTE.h"
#include "PageDirectory.h"		// BASEADDRESS
#include "IO/OStream.h"

namespace MyOS {
namespace MM {

// Calculates the linear address mapped by this PTE
inline linadr_t PTE::getLinearAddress() const
{
	return (linadr_t) (((u32)this - (u32)PageDirectory::BASEADDRESS) << 10);
}

s32 
PTE::list( IO::OStream &o, u32 rangecount ) const
{
    u32 start = o.getMarker();
    if (isMapped())
		o.printf( 
            "\n<MAPPING lin=\"%x\" phys=\"%x\" atts=\"%x\" pages=\"%d\"/>",
		    this->getLinearAddress(), getMapping(), getAttributes(), rangecount );
	else
		o.printf( 
            "\n<UNMAPPED lin=\"%x\" pages=\"%d\"/>", 
            this->getLinearAddress(), rangecount );
	return o.getMarker() - start;
}

}}  // namespace
