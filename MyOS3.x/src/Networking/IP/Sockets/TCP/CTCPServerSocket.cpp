/***************************************************************************
                          CTCPServerSocket.cpp  -  description
                             -------------------
    begin                : Fri Jan 11 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain

TO DO: Built-in thread pooling, size configurable

 ***************************************************************************/
#include "CTCPServerSocket.h"
#include "MM/new.h"
#include "CTCPSocket.h"
#include "TCPHeader.h"
#include "Networking/IP/IPPacket.h"

#include "Services/Network/TCP/TCPService.h"		// for new

// debugging
#include "debug.h"

// Need this one! For some reason...
// using MyOS::Networking::IP::TCP::CTCPServerSocket;
//inline void operator delete( void* s ) {
//	MyOS::Services::Network::TCP::operator delete( s, sizeof(CTCPServerSocket) );
//}

namespace MyOS { namespace Networking { namespace IP { namespace TCP {

using namespace MyOS::Services::Network::TCP;

CTCPServerSocket::CTCPServerSocket( 
	IIPEndpoint &ep, ipport_t port, ITCPServerSocketClient *cl )
	: ITCPServerSocket(ep,port),  
	pendingConnections(0), pendingCount(0), acceptedConnections(0),
	maxQueueLength(0), client(cl)
{

}

/*
CTCPServerSocket::~CTCPServerSocket()
{
   // close() ?
}

void 
CTCPServerSocket::operator delete( void* p )
{
	deallocate( p, sizeof(CTCPServerSocket) );	
}
*/


myos_result_t
CTCPServerSocket::listen( u32 maxq )
{
	DPRINTK( "\nTCPServerSocket::listen port=%u queue=%u", getLocalPort(), maxq );
	maxQueueLength = maxq;
	return getEndpoint().addSocket( *this );
}

void	// virtual, callback made here to reduce stack growth
CTCPServerSocket::onPacket( IPPacket &p ) {
	if (client && receive(p)) {
		const sockadr_t src( p.getSource(), p.getData<TCPHeader>().getSourcePort() );
		client->onConnection( *this, src );
	}
}

// to avoid growing the stack too much, the caller must do the callback
//
// PRE: TCP packet SYN flag is set
bool
CTCPServerSocket::receive( IPPacket &p )
{
	// LOG( C_IPSocketLayer,  "CTCPServerSocket::receive" );

	if (pendingCount < maxQueueLength ) {	// socket open (maxQ>0), still room
		TCPHeader &tcph = p.getData<TCPHeader>();
		if (tcph.isReset()) {
			// LOG( C_IPSocketLayer,  "..ignoring incoming RST" );
			return false;		/// RFC793, p65: ignore incoming RST
		}
		
		// Assume: tcph.isSYN()

		// Check that no connection to that <remote_IP,remote_port> exists!
		for (CTCPSocket* s = acceptedConnections; s!=0; s=s->next) {
			// XX needless byteswaps here... use nw16_t types
			if ( s->getRemoteAddress() == p.getSource() && 
				 s->getRemotePort()    == tcph.getSourcePort() ) {
				DPRINTK( "\nCTCPServerSocket: received SYN for existing socket: %x:%d pkt %x:%d",
					s->getRemoteAddress().host(), s->getRemotePort(),
					p.getSource().host(), tcph.getSourcePort() );
				s->onPacket(p);
				return false;	// not to local socket client
			}
		}		

		// Add connection to queue (before callback) because client will call accept()
		CTCPSocket *s = new CTCPSocket( getEndpoint(), p, *this );
		s->next = pendingConnections;
		pendingConnections = s;
		++pendingCount;

		if (client) {
			// LOG( C_IPSocketLayer, "CTCPServerSocket::receive calling async client::onConnection..." );
			// note: bool ignored	DANGER: should not block below in accept() ...
			// Called in onPacket instead
			// client->onConnection( *this, sockadr_t( p.getSource(), tcph.getSourcePort() ) );
			return true;
		} else {
			// Synchronous, no switch now ?
			sync.signal(); 
		}

	} else DPRINTK( "...too many pending connections, ignoring" );

	return false;
}

/**
 * 	DANGER!! Too many nested calls will fill the stack very quickly !
 */
ITCPSocket&
CTCPServerSocket::accept( ITCPSocketClient* client, u32 timeout /*=0*/ )
    throw (Exceptions::SocketException)
{
	// LOG( C_IPSocketLayer,  "CTCPServerSocket::accept stackleft=%u",
	//	Processor::ESP() - (Processor::ESP() & 0xFFFFF000) );

	// check if there is a pending connection, else block
	while (pendingConnections==0) {
		if ( !sync.waitUs( 1000 * timeout ) ) {
			DPRINTK( "\nCTCPServerSocket::accept timeout" );
			throw Exceptions::SocketException( Exceptions::E_TIMEOUT );
		}
		if (maxQueueLength==0) throw Exceptions::SocketException( Exceptions::E_QUEUE_UNDERRUN );
	}

	CTCPSocket* s = pendingConnections; 
	pendingConnections = s->next; 
	--pendingCount;
	if (s->accept(client)==false) {
      s->destroy();
      throw Exceptions::SocketException( Exceptions::E_ACCEPT_ERROR );
	}
	
	s->next = acceptedConnections;
	acceptedConnections = s;
	return *s;
}

myos_result_t
CTCPServerSocket::close()
{
	DPRINTK( "\nCTCPServerSocket::close" );
	maxQueueLength=0;

	getEndpoint().removeSocket( *this );

	// cancel any pending accept()
	sync.signal();
	
	// TODO: Close all open client sockets?
	
	if (client) client->onClose( *this );
	return E_MYOS_SUCCESS;
}

myos_result_t
CTCPServerSocket::destroy()
{
	close();
	delete this;
	return E_MYOS_SUCCESS;
}

void
CTCPServerSocket::onSocketClose( CTCPSocket& closed )
{
	for (CTCPSocket* s = acceptedConnections, *prev=0; s; prev=s, s=s->next) {
		if ( s == &closed ) {
			if (prev) 
				prev->next = closed.next;
			else 
				acceptedConnections = 0;
			return;
		}
	}		
}

}}}}	// namespace
