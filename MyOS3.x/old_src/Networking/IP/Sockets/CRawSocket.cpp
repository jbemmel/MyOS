/***************************************************************************
                          C_RawSocket.cpp  -  description
                             -------------------
    begin                : Fri Nov 16 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "CRawSocket.h"
#include "../IPPacket.h"
#include "buffer.h"
#include "../CIPEndpoint.h"
#include "MM/new.h"

namespace MyOS { namespace Networking { namespace IP {
	
CRawSocket::CRawSocket( CIPEndpoint& ep, E_IPPROTOCOL proto, IRawSocketClient* cl )
: IRawSocket(ep,proto), client(cl)
{

}

/*
void*
CRawSocket::operator new( size_t )
{
	return allocate( sizeof(CRawSocket) );
}


void
CRawSocket::operator delete( void *p )
{
	deallocate( p, sizeof(CRawSocket) );
}
*/

// virtual
void
CRawSocket::onPacket( IPPacket& packet )
{
   // save remote IP address (not port) for response
   setRemoteAddress( packet.getSource(), 0 );

   if (client) {
      client->onRecv( *this, packet.getSource(), packet );
   } else {
   		PRINTK( "\nRAWSocket onPacket: no client!" );
   }
}

// virtual
myos_result_t
CRawSocket::open( options_t options ) {
   return endpoint.addSocket( *this );
}

// virtual
myos_result_t
CRawSocket::connect( ipadr_t to )
{
   myos_result_t r = open(0); // implies open, do this before setting remote address
   setRemoteAddress( to, 0 );
   return r;
}
   /** @see CAbstractSocketBase::close() */
// virtual
myos_result_t
CRawSocket::close()
{
   // make sure that hashkey equals open() -> remove remote info
   setRemoteAddress( 0, 0 );
   return endpoint.removeSocket( *this );
}

   /** @see CAbstractSocketBase::destroy() */
// virtual
myos_result_t
CRawSocket::destroy()
{
   close();
   delete this;
   return E_MYOS_SUCCESS;
}

/// @return number of bytes sent, or (negative) error on failure
// virtual
myos_result_t
CRawSocket::send( const buffer& data, options_t flags )
{
   return sendTo( getRemoteAddress(), data, flags );
}

// virtual, almost identical to CUDPSocket::sendTo except for header size
myos_result_t
CRawSocket::sendTo( ipadr_t to, const buffer& data, options_t flags )
{
	size_t payload = data.getSize();
	IPPacket *p = endpoint.createPacket( payload );
	if (p) {
		memcpy( &p->getData<u8>(), data.getData(), payload );
		myos_result_t result = sendPacketTo( to, *p, payload, flags );
		// endpoint.destroyPacket( *p );
		return result;
	} else return E_MYOS_ENOMEM;
}

// virtual
myos_result_t
CRawSocket::recv( const buffer& inbuf, options_t flags )
{ return E_MYOS_E_NOTIMPL; }

// virtual
myos_result_t
CRawSocket::recvFrom( ipadr_t from, const buffer& inbuf, options_t flags )
{ return E_MYOS_E_NOTIMPL; }

// virtual
myos_result_t
CRawSocket::sendPacket( IPPacket& p, size_t payload, options_t flags ) {
   return sendPacketTo( getRemoteAddress(), p, payload, flags );
}

// virtual
myos_result_t
CRawSocket::sendPacketTo( ipadr_t to, IPPacket& p, size_t payload, options_t flags )
{
	// PRINTK( "\nRAWSocket::sendPacketTo %x", to.host() );	
	
	// set IP options
	p.setTTL( getTTL() );
	p.setTOSbits( getTOSbits() );
	return endpoint.send( p, to, payload, getProtocol(), flags );
}

// virtual
myos_result_t
CRawSocket::recvPacket( IPPacket& p, size_t maxpayload, options_t flags )
{
   return E_MYOS_E_NOTIMPL;
}

// virtual
myos_result_t
CRawSocket::recvPacketFrom( ipadr_t from, IPPacket& p, size_t maxpayload, options_t flags )
{
   return E_MYOS_E_NOTIMPL;
}

}}}   // namespace
