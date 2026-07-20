/***************************************************************************
                          CTCPSocketFactory.h  -  description
                             -------------------
    begin                : Wed Oct 9 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#include "CTCPSocketFactory.h"

#include "Services/Network/TCP/TCPService.h"

// instance classes
#include "CTCPSocket.h"
#include "CTCPServerSocket.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {

using namespace MyOS::Services::Network::TCP;

/*
void
CTCPSocketFactory::operator delete( void* p )
{
	deallocate( p, sizeof(CTCPSocketFactory) );	
}*/

void* CTCPSocket::operator new( size_t )
{
    return allocate( sizeof(CTCPSocket) );
}

void CTCPSocket::operator delete( void* p ) throw()
{
    deallocate( p );
}

// virtual
ITCPSocket*
CTCPSocketFactory::createTCPSocket(
   IIPEndpoint& endpoint, ITCPSocketClient *client )
{
   return new CTCPSocket( endpoint, client );
}

// virtual
ITCPServerSocket*
CTCPSocketFactory::createTCPServerSocket(
   IIPEndpoint& endpoint, ipport_t localport, ITCPServerSocketClient* client )
{
   return new CTCPServerSocket( endpoint, localport, client );
}

}}}} // namespace
