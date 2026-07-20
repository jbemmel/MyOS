#include "FATBootSector.h"

#include "debug.h"
#include "types.h"

namespace MyOS { namespace FS { namespace FAT {

#ifdef DEBUG
void
FATBootSector::dump() const
{
// DPRINTK( "\nVendor = %s", vendor ); // not zero-terminated!
DPRINTK( "\nbytesPerSector    = %x", bytesPerSector );
DPRINTK( "\nsectorsPerCluster = %x", sectorsPerCluster );
DPRINTK( "\nreservedSectors   = %x", reservedSectors );
DPRINTK( "\nnumberOfFATs      = %x", numberOfFATs );
DPRINTK( "\nrootEntries       = %x", rootEntries );
DPRINTK( "\nlogicalSectors    = %x", logicalSectors );
DPRINTK( "\nmediumDescriptor  = %x", mediumDescriptor );
DPRINTK( "\nsectorsPerFAT     = %x", sectorsPerFAT );
DPRINTK( "\nsectorsPerTrack   = %x", sectorsPerTrack );
DPRINTK( "\nnumberOfHeads;    = %x", numberOfHeads );
DPRINTK( "\nhiddenSectors;    = %x", hiddenSectors );
   
}

#endif
  
}}}   // namespace
