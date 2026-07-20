/***************************************************************************
                          TCPCloseWait.h  -  description
                             -------------------
    begin                : Sun Nov 18 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain

State pattern, see e.g. Design Patterns

 ***************************************************************************/
#ifndef TCPCloseWait_H
#define TCPCloseWait_H

#include "ITCPState.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
/**
 * 	CloseWait: ESTABLISHED -> Receive FIN -> wait for local Close()
 */
class TCPCloseWait : public ITCPState
{
public:
	static ITCPState* instance() { static TCPCloseWait _instance; return &_instance; }

	/// I_TCPState methods
	virtual void receive( CTCPSocket &s, TCPHeader &tcppacket, size_t ippayload );
	virtual s32 send( CTCPSocket &s, TCPHeader &packet, size_t tcppayload );
	virtual myos_result_t close( CTCPSocket &s );

	/// Invalid methods
	virtual bool listen( CTCPSocket &s ) { return false; }

private:
	TCPCloseWait() {}
};

}}}}	// namespace

#endif
