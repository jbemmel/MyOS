/***************************************************************************
                          TCPClosing.h  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain

State pattern, see e.g. Design Patterns

 ***************************************************************************/
#ifndef TCPClosing_H
#define TCPClosing_H

#include "ITCPState.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {

/**
 * CLOSING - represents waiting for a connection termination request
 *           acknowledgment from the remote TCP.
 */
class TCPClosing : public ITCPState
{
public:
	static ITCPState* instance() { static TCPClosing _instance; return &_instance; }

	/// ITCPState methods
	virtual void receive( CTCPSocket &s, TCPHeader &tcppacket, size_t ippayload );

	virtual myos_result_t close( CTCPSocket &s );

	/// Invalid methods
	virtual bool listen( CTCPSocket &s ) { return false; }

private:
	TCPClosing() {}
};

}}}}	// namespace

#endif
