#include "PageDirectory.h"
#include "IO/OStream.h"

namespace MyOS {
namespace MM {

s32
PageDirectory::listTable( IO::OStream &o ) const
{
   // get_PDT_ is correct here!
	o.printf( "\n<PAGETABLE PBTindex=\"%d\">", getPDTIndex(this) );
	u32 i=0, count=0;
	do {
		u32 rangestart = i;
		u32 mapping = entries[i].getMapping();
		/// Collect (range of) mapped & adjacent entries, count them
		while (entries[i].isMapped())
		{
			++count;
			if (++i>=1024 || entries[i].getMapping() != (mapping+=_4KB)) break;
		}
		if (rangestart!=i) {
			entries[rangestart].list(o, i-rangestart);
			rangestart = i;
		}
		/// Collect (range of) unmapped entries
		while ( i<1024 && !entries[i].isMapped()) ++i;
		if (rangestart!=i) {
			entries[rangestart].list(o, i-rangestart);
		}
	} while (i<1024);

	o.printf( "\n</PAGETABLE>" );
	return count;
}

}}  // namespace
