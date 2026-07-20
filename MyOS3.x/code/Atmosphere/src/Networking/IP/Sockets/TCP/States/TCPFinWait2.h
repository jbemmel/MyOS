/***************************************************************************
                          TCPFinWait2.h  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef TCPFinWait2H
#define TCPFinWait2H

#include "ITCPState.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
class TCPFinWait2 : public ITCPState
{
public:
	static ITCPState* instance() { static TCPFinWait2 instance; return &instance; }

	/// ITCPState methods
	virtual void receive( CTCPSocket &s, TCPHeader &tcppacket, size_t ippayload );

	virtual myos_result_t close( CTCPSocket &s );

	/// Invalid calls
	virtual bool listen( CTCPSocket &s ) { return false; }

private:
	inline TCPFinWait2() : ITCPState("FINWAIT-2") {}
};

}}}}	// namespace

#endif
