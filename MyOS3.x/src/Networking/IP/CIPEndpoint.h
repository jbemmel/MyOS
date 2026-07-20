/***************************************************************************
                          CIPEndpoint.h  -  description
                             -------------------
    begin                : Fri Jun 28 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef CIPENDPOINT_H
#define CIPENDPOINT_H

#include "Networking/IP/IPComponent.h"
#include "IIPEndPoint.h"
#include "Sockets/SocketBase.h"   // KEY
#include "HASHTABLE.h"

namespace MyOS { namespace Networking { namespace IP {
	
using Util::HASHTABLE;

class IIPLayerImpl;

class CIPEndpoint : public IIPEndpoint
{
public:
   // should take an IDevice& as parameter
   CIPEndpoint( ipadr_t local, ipadr_t subnet, IIPLayerImpl& l );

   // IIPEndpoint
   virtual void destroy() = 0;

   /**
    * @note This will only work if _routing_ can already be determined for this packet (!)
    * If it can go either to a serial link or an ethernet card, this class can never determine
    * the size of memory needed...
    */
	virtual IPPacket* createPacket( size_t maxDataSize ) = 0;
	// virtual void destroyPacket( IPPacket &p ) = 0;

	// virtual myos_result_t send( IPPacket& packet, ipadr_t to, size_t payload, E_IPPROTOCOL p ) = 0;
	// virtual myos_result_t changeAddress( ipadr_t local, ipadr_t subnet ) = 0;

    virtual ITCPServerSocket* createTCPServerSocket(
        ipport_t localport, ITCPServerSocketClient* client );
    virtual ITCPSocket* createTCPSocket(
      ITCPSocketClient *client );
    virtual IDatagramSocket* createUDPSocket(
      IDatagramSocketClient *client, ipport_t localport /*=0*/) ;
    virtual IRawSocket* createRawSocket(
		E_IPPROTOCOL proto, IRawSocketClient *client );


	virtual void listSockets( IO::OStream& out ) const;

   // called by sockets
   virtual myos_result_t addSocket( CAbstractSocketBase& sock );
   virtual myos_result_t removeSocket( CAbstractSocketBase& sock );

protected:
   void receive( IPPacket &ip, size_t packetsize );
   void destroyEndpoint();
   myos_result_t changeAddressInternal( ipadr_t local, ipadr_t subnet );
   virtual myos_result_t clearAddress();

private:
	enum { LOG2MAX=5, MAXSOCKETS=1<<LOG2MAX };
	typedef HASHTABLE< CAbstractSocketBase::KEY, CAbstractSocketBase, LOG2MAX > SOCKETSET;

	void unhandledTCPPacket( IPPacket &ip );

	IIPLayerImpl& layer;
	
	// TODO: This is the wrong datamodel, sockets may span multiple interfaces
	// E.g. one serversocket may listen for incoming connections on all
	// or 'bind()' to specific IP address (multiple?)
	SOCKETSET sockets;		
};

}}} // namespace

#endif
