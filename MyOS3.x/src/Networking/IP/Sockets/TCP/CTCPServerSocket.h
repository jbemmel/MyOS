/***************************************************************************
                          CTCPServerSocket.h  -  description
                             -------------------
    begin                : Thu Jan 10 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain

Serversockets are listening TCP sockets waiting for incoming connections. They are 'half'
in the sense that remote IP+port are not known, which implies that HashTable lookup
must not use the remoteInfo (to process incoming packets)

Operation
---------
After creation, a ServerSocket is put into 'listening' mode. When a connection request
comes in, a true TCPSocket is created and the connection is put in a queue. If the server
application is blocking on accept(), it will be awakened and the new socket is added
to the hashtable; a reply is sent. If not, a timer is started and the connection may
time out
 ***************************************************************************/
#ifndef CSERVERSOCKET_H
#define CSERVERSOCKET_H

#include "ITCPServerSocket.h"
#include "MultiThreading/IMultiThreading.h"
#include "MultiThreading/Synchronization/CSyncPoint.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
	
class CIPEndpoint;
class CTCPSocket;

class CTCPServerSocket : public ITCPServerSocket
{
	// packet reception function
	virtual void onPacket( IPPacket& packet );

	// some constant parameter defaults, TBD: move to interface
	enum { MAXPENDINGCONNECTIONS = 10 };

public:
	CTCPServerSocket( IIPEndpoint &ep, ipport_t p, ITCPServerSocketClient *cl );	
	// ~CTCPServerSocket();
	// void operator delete( void* );

	virtual myos_result_t destroy();
	virtual myos_result_t close();
	virtual myos_result_t listen( u32 maxConnectionsInQueue = MAXPENDINGCONNECTIONS );
	virtual ITCPSocket& accept( ITCPSocketClient* client, u32 timeout /*=0*/ )
	    throw (Exceptions::SocketException);

	// incoming packet processing
	bool receive( IPPacket &packet );

	inline ITCPServerSocketClient* getClient() const { return client; }

	// called by CTCPSocket
	void onSocketClose( CTCPSocket& s );

private:
	friend class CTCPSocketFactory;

	CTCPSocket* pendingConnections;
	size_t pendingCount;	// could be atomic32_t

	// Keep track of accepted connections, in order to be able to direct
	// duplicate SYNs to the right socket for processing
	CTCPSocket* acceptedConnections;

	// max. size of queue before dropping new requests, 0 means socket is closed, >0 listening
	u32 maxQueueLength;

	// Syncpoint for server application to block on accept()
	MultiThreading::Synchronization::CSyncPoint sync;
	ITCPServerSocketClient *client;		// optional async callback
};

}}}}	// namespace

#endif
