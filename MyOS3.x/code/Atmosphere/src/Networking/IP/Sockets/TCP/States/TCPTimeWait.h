/***************************************************************************
                          TCPTimeWait.h  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain

State pattern, see e.g. Design Patterns

 ***************************************************************************/
#ifndef TCPTimeWait_H
#define TCPTimeWait_H

#include "ITCPState.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
class TCPTimeWait : public ITCPState
{
public:
	static ITCPState* instance() { static TCPTimeWait _instance; return &_instance; }

	/// ITCPState methods
	virtual void receive( CTCPSocket &s, TCPHeader &tcppacket, size_t ippayload );
	virtual void onTimer( CTCPSocket &s );

	/// Invalid methods
	virtual myos_result_t close( CTCPSocket &s ) { return E_MYOS_E_FAIL; };
	virtual bool listen( CTCPSocket &s ) { return false; }

	// Should be used by other states when entering this state, starts timer
	static void enter( CTCPSocket &s );

private:
	inline TCPTimeWait() : ITCPState("TIMEWAIT") {}
};

}}}}	// namespace

#endif
