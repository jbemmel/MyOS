#include "iptypes.h"
#include "OStream.h"

namespace MyOS { namespace Networking { namespace IP {

void
ipadr_t::print( IO::OStream& out ) const
{
	for (int b=0; b<4; ++b)	{
		register u8 n = u.byte[b];
		if ( n > 99) {
			out.put( n/100 + '0' );
		}
		if ( n > 9 ) {
			out.put( (n/10)%10 + '0' );
			n %= 10;
		}	
		out.put( n + '0' );
		if (b<3) out.put('.');		
	}
}
	
}}}	// namespace
