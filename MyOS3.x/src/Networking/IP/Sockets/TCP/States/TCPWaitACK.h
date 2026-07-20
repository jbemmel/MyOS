/***************************************************************************
                          TCPWaitACK.h  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain

State pattern, see e.g. Design Patterns

 ***************************************************************************/
#ifndef TCPWaitACK_H
#define TCPWaitACK_H

#include "ITCPState.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {

class TCPWaitACK : public ITCPState
{
public:
	static ITCPState* instance() { static TCPWaitACK _instance; return &_instance; }

	/// ITCPState methods
	virtual void receive( CTCPSocket &s, TCPHeader &tcppacket, size_t ippayload );

	/// Invalid methods
	virtual myos_result_t close( CTCPSocket &s ) { return E_MYOS_E_FAIL; };
	virtual bool listen( CTCPSocket &s ) { return false; }

private:
	inline TCPWaitACK() : ITCPState("WAIT-ACK") {}
};

}}}}	// namespace

#endif
