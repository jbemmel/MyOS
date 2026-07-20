/***************************************************************************
                          TCPListen.h  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain

State pattern, see e.g. Design Patterns

 ***************************************************************************/
#ifndef TCPListen_H
#define TCPListen_H

#include "ITCPState.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {

class TCPListen : public ITCPState
{
public:
	static TCPListen* instance() { static TCPListen _instance; return &_instance; }

	/// I_TCPState methods
	virtual void receive( CTCPSocket &s, TCPHeader &tcppacket, size_t ippayload );
	virtual myos_result_t close( CTCPSocket &s );

	// invalid calls
	virtual bool listen( CTCPSocket &s ) { return false; }

	// complete the sequence started with receive()
	bool accept( CTCPSocket &s );

private:
	inline TCPListen() : ITCPState("LISTEN") {}
};

}}}}	// namespace

#endif
