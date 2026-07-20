/***************************************************************************
                          ServerSocket.h  -  description
                             -------------------
    begin                : Mon Feb 4 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#include "types.h"
#include "../SocketBase.h"

namespace MyOS { namespace Networking { namespace IP {
	
class ITCPSocket;				
class ITCPSocketClient;

/**
 * ServerSockets are an abstraction commonly used in combination with a 
 * connection-oriented protocol like TCP
 */
class ITCPServerSocket : public CAbstractSocketBase
{
public:
	/**
	 * 	Indicate willingness to accept incoming connections
       *    @param maxConnectionsInQueue : max. number of pending connections, must be >0
	 */
	virtual myos_result_t listen( u32 maxConnectionsInQueue=5 ) = 0;

	/**
	 * 	Synchronously accept an incoming connection, usually called from 
     *    ITCPServerSocketClient::onConnection()
	 *	accept() will block until receiving an incoming connection or timeout.
	 *	Then a new connected TCP socket is created with the remote address of the 
     *    incoming connection, and passed in the 'socket' parameter
	 *
     *  @param socket  : [out] resulting socket for communication, 0 if error
	 *	@param client  : callback interface for async. operation
	 *	@param timeout : max time in ms to wait before returning, 0 for indefinite
	 */
	virtual myos_result_t accept( 
      ITCPSocket*& socket, ITCPSocketClient* client, u32 timeout=0 ) = 0;

	/**
	 * 	Cancel any pending connections, and stop accepting packets
    *    @see CAbstractSocketBase::close()
	 */
	virtual myos_result_t close() = 0;

	/**
	 * 	Close the socket and free all resources associated with this serversocket
    *    @see CAbstractSocketBase::destroy()
	 */
	virtual myos_result_t destroy() = 0;

protected:
   ITCPServerSocket( IIPEndpoint& ep, ipport_t port)
      : CAbstractSocketBase(ep,E_IPPROTO_TCP,port) {}
};

/**
 * 	Async callback interface
 */
class ITCPServerSocketClient
{
public:
	// should pass initial data? or perhaps after acception
	virtual bool onConnection( ITCPServerSocket &on, const sockadr_t& from ) = 0;

	// optional
	virtual void onClose( ITCPServerSocket& onSocket ) {}
};

}}}	// namespace

#endif
