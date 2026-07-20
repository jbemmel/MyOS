//
//	Additional (custom) datastructures for PCI management
//

#ifndef PCI_RESOURCE_INFO_H
#define PCI_RESOURCE_INFO_H

#include "types.h"

namespace MyOS {
namespace Drivers {
namespace PCI {

// helper structure to communicate resource sizes
struct PCI_RESOURCE_INFO
{
    u32 minMEM, maxMEM;	    // in bytes
    u16 minIO, maxIO;
	
	PCI_RESOURCE_INFO() : minMEM(0xFFFFFFFF), maxMEM(0), minIO(0xFFFF), maxIO(0) {}
	
	inline void include( const PCI_RESOURCE_INFO &res )
	{
		// values of 0 are invalid for minimum values
		if (res.minIO && (minIO > res.minIO)) minIO = res.minIO;
		if (maxIO < res.minIO) maxIO = res.maxIO;
		if (res.minMEM && (minMEM > res.minMEM)) minMEM = res.minMEM;
		if (maxMEM < res.maxMEM) maxMEM = res.maxMEM;
	}
	
	inline u16 getIOSize() const	{ return maxIO ? (maxIO-minIO) : 0; }
	inline u32 getMEMSize() const	{ return maxMEM ? (maxMEM-minMEM) : 0; }
} PACKED;

}}}	// namespace

#endif
