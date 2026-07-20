#include "TCPHeader.h"
#include "../../IPPacket.h"

#include "debug.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
	
size_t
TCPHeader::createReset( ) // assume: h is packet received (!)
{
	IPPacket *iph = ((IPPacket*) this) - 1;

	// TCP specs p.36, reusing incoming TCP packet
	if (hasACK()) {
		setSeq( getACK() );
		setFlags( TCP_RESET );
	} else {
      // seglength, include SYN, include FIN ?
		setACK( getSeq() + iph->getPayloadSize() - getHSizeBytes() + isSYN() );
		setSeq( 0 );
		setFlags( TCP_RESET | TCP_ACK );
	}

	// Determine addressing
	ipadr_t from 	= iph->getDestination();
	ipadr_t to 		= iph->getSource();

	// swap ports, assuming p was from source
	setPorts( getDestinationPort(), getSourcePort() );

	// Finish TCP header
	setHSizeBytes( 5 * sizeof(u32) );		// basic, ignore options
	wrapUp( from, to, sizeof(TCPHeader) );

	// Finish IP header
	iph->wrapUp( from, to, sizeof(TCPHeader), E_IPPROTO_TCP );
	return sizeof(TCPHeader);
}

enum TCPOPTIONKIND {
	TCP_KIND_ENDOFOPTION	   = 0,
	TCP_KIND_NOOP				= 1,
	TCP_KIND_MAXSEGSIZE		= 2,
	TCP_KIND_WINDOWSCALE 	= 3
};

u32	// or'ed bitmask of options found
TCPHeader::parseOptions( TCPOPTIONS &result ) const
{
	/// Only applies to SYN segments
	if (!isSYN()) return 0;

	register const u8 *options = (const u8*) (((u8*)this) + sizeof(TCPHeader));
	const u8 *optionend = getData<u8>();
	u32 fields = 0;
	while ( options < optionend ) {
next:
			switch (*options)
			{
			case TCP_KIND_ENDOFOPTION:
				return fields;

			case TCP_KIND_NOOP:
				++options;
				goto next;		// dont add options[1]

			case TCP_KIND_MAXSEGSIZE:
				result.maxsegsize = htons16( * ((u16*) &options[2]) );
				fields |= TCP_MSS;
				break;

			case TCP_KIND_WINDOWSCALE:
				// RFC 1323: Only valid in SYN segment, ACK=0 or recv before
				if (!hasACK() || result.windowscale) {
					result.windowscale = options[2];
					fields |= TCP_WINDOWSCALE;
				}
				break;

			default:
				break;	// ignore
			}

         // guard against invalid length value 0 (RFC 1122), should RST
         if (options[1]==0) return E_TCP_INVALID_OPTION_LENGTH;
			options += options[1];	// skip over option by using length field
	};
	return fields;
}

bool
TCPHeader::verifyChecksum( ipadr_t from, ipadr_t to, size_t totaloctets )
{
	u16 original = checksum;
	calcChecksum(from,to,totaloctets);
	if (original!=checksum) {
		DPRINTK( "\nTCP checksum mismatch: orig=%x mine=%x packetsize=%d",
			original, checksum, totaloctets );
		return false;
	} else return true;
}

/// Bytemasks for processing trailing 1..3 bytes
static const u32 bytemasks[] = { 0x000000FF, 0x0000FFFF, 0x00FFFFFF };

/// From linux asm/checksum.h (similar to IP checksum)
void
TCPHeader::calcChecksum( ipadr_t from, ipadr_t to, size_t totaloctets )
{
	/// first clear the checksum field
	checksum = 0;

	// This is somewhat tricky: TCP requires inclusion of a 12-byte pseudo-header
	u32 sum = htons16(totaloctets) | (HTONS16(E_IPPROTO_TCP)<<16);

	// If the number of bytes%4!=0, add the tailbytes here (there can be no carry)
	if (totaloctets&3) {
		sum += ((u32*)this)[ totaloctets>>2 ] & bytemasks[ (totaloctets&3)-1 ];
	}

	// These are 8 more bytes for the pseudo header
	u32 f = from.nw();
	u32 t = to.nw();
	ASMVOLATILE(
		"adcl %1, %0;"
		"adcl %2, %0;"
		"adcl $0, %0;"
		: "=r"(sum), "=r"(f), "=r"(t)
		: "0"(sum),  "1" (f), "2" (t)
	);

	TCPHeader *h  = this;
	size_t dwords = (totaloctets>>2); 	/// in dwords, round down
	ASMVOLATILE(
       "clc	            ;"
"1:"
        "adcl (%1), %0	;"
	    "lea 4(%1), %1	;"
	    "decl %2		;"
	    "jne	1b		;"
	    "adcl $0, %0	;"
	    "movl %0, %2	;"
	    "shrl $16, %0	;"
	    "addw %w2, %w0	;"
	    "adcl $0, %0	;"
	    "notl %0		;"
"2:"

	/// Must specify registers in output as well...
	: "=r"(sum), "=r"(h), "=r"(dwords)
	: "0"(sum),  "1" (h), "2" (dwords)
	);
	checksum = sum;
}

}}}}
