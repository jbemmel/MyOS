/***************************************************************************
                          TCPSynSent.h  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef TCPSynSent_H
#define TCPSynSent_H

#include "ITCPState.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {

/**
 * 	Non-synchronized state after TCPClosed, waiting for connection to be accepted (SYN)
 */
class TCPSynSent : public ITCPState
{
public:
	static ITCPState* instance() { static TCPSynSent _instance; return &_instance; }

	/// I_TCPState methods
	virtual void receive( CTCPSocket &s, TCPHeader &tcppacket, size_t ippayload );
	virtual myos_result_t close( CTCPSocket &s );

	/// Invalid calls
	virtual bool listen( CTCPSocket &s ) { return false; }

private:
	inline TCPSynSent() : ITCPState("SYNSENT") {}
};

}}}}	// namespace

#endif
