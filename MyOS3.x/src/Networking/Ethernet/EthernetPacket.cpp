#include "EthernetPacket.h"

#include "debug.h"

namespace MyOS { namespace Networking { namespace Ethernet {

const ethadr_t EthernetPacket::BROADCAST = {{0xff,0xff,0xff,0xff,0xff,0xff}};

void EthernetPacket::debugDump( size_t payload )
{
	u8* p = (u8*)(this+1);
	DPRINTK( "\nEth payload:" );
	for (size_t d=0; d<payload; ++d) {
		DPRINTK("%X", p[d]);	
	}
	DPRINTK( "\n------------" );
}

}}}	// namespace
