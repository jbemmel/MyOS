/***************************************************************************
                          TCPFinWait2.cpp  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "TCPFinWait2.h"
#include "../TCPHeader.h"

/// State transitions
#include "TCPEstablished.h"	// code sharing
#include "TCPTimeWait.h"

#include "debug.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
	
void	// virtual
TCPFinWait2::receive( CTCPSocket &s, TCPHeader &p, size_t ippayload )
{
	DPRINTP( "\nTCPFinWait2::receive" );

	// 2. Check RST bit
	if (p.isReset()) {
		// this destroys the socket
		return s.receiveRST();
	}

	// 3. Check security/precedence, TBD

	// 4. check SYN
	if (p.isSYN()) {
		return s.receiveSYN(p);
	}

	// 5. check ACK
	if (p.hasACK()) {
		if (s.receiveACK(p)) {
			// if retransmission queue empty, ACK local close() now
			// dont destroy socket yet
		} else return;	// drop
	} else return;	// drop

	// 6. Check URG, TBD
	
	// 7. Process data, this may destroy the packet! save the FIN flag!
	bool isFIN = p.isFinish();
	s.receiveData(p,ippayload);

	// 8. Check FIN
	if (isFIN) {
		s.receiveFIN();
		TCPTimeWait::enter(s);
	}
}

myos_result_t
TCPFinWait2::close( CTCPSocket &s )
{
	DPRINTK( "\nTCPFinWait2::close" );
	return E_MYOS_SUCCESS;
}

}}}} // namespace
