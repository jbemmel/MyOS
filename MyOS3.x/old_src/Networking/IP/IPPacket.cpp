#include "IPPacket.h"
#include "debug.h"

namespace MyOS { namespace Networking { namespace IP {

u16	// static
IPPacket::sUniqueID = 0x1111;	// little->big-endian conversion gives effects..

/// From linux asm/checksum.h
inline u16	// inline (only called from below)
IPPacket::ip_fast_csum()
{
	/// first clear the checksum field (in case packet is reused)
	headerChecksum = 0;

	u32 sum;                            /// initialized in ASM
	IPPacket *h = this;
	size_t headerdwords = ver_hsize & 0x0F;   /// in dwords

	ASMVOLATILE (
	    "movl (%1), %0	 ;"
	    "subl $4, %2	 ;"
	    "jbe 2f			 ;"
	    "addl 4(%1), %0	 ;"
	    "adcl 8(%1), %0  ;"
	    "adcl 12(%1), %0 ;"
"1:	     adcl 16(%1), %0 ;"
	    "lea 4(%1), %1	 ;"
	    "decl %2		 ;"
	    "jne	1b       ;"
	    "adcl $0, %0	 ;"
	    "movl %0, %2	 ;"
	    "shrl $16, %0	 ;"
	    "addw %w2, %w0	 ;"
	    "adcl $0, %0	 ;"
	    "notl %0		 ;"
"2:"
	/// Must specify registers in output as well...
	: "=r"(sum), "=r"(h), "=r"(headerdwords)
	: "1" (h),   "2" (headerdwords)
	);
	return(sum);
}


void
IPPacket::wrapUp( ipadr_t from, ipadr_t to, size_t payload, E_IPPROTOCOL p )
{
	totalLength = ( payload + sizeof(IPPacket) );
	identification = ++sUniqueID;
	flagsOffset = 0;	// Needs clearing! TODO: Support fragmentation?
	protocol = p;
	source = from;
	destination = to;
	
	
	// TTL = 0x40;  // set here to avoid wrong values in reused packets ?

	/// Must be done last :)
	headerChecksum = ip_fast_csum();
}

bool
IPPacket::verifyChecksum()
{
	u16 originalCSum = headerChecksum;
	return originalCSum == ip_fast_csum();	
}

void
IPPacket::debugDump( const char* byteformat )
{
	u8* p = (u8*)(this/*+1*/);
	DPRINTK( "\nIP header:" );
	for (u32 d=0; d<sizeof(IPPacket); ++d) {
		DPRINTK(byteformat, p[d]);	
	}
	DPRINTK( "\npayload:" );
	for (u32 d=sizeof(IPPacket); d<(getTotalSize()); ++d) {
		DPRINTK(byteformat, p[d]);	
	}	
	DPRINTK( "\n------------" );	
}

}}}  // namespace
