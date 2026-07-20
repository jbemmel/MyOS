/***************************************************************************
                          DHCPClient.h  -  description
                             -------------------
    begin                : Tue Sep 10 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef IDHCPCLIENTIMPL_H
#define IDHCPCLIENTIMPL_H

#include "MultiThreading/Synchronization/CSyncPoint.h"    // syncpoint
#include "IDHCPClient.h"
#include "Networking/IP/Sockets//IDatagramSocket.h"
#include "Devices/Ethernet.h"       // not the best place to put ethadr_t

#include "DHCPPacket.h"             // options struct

namespace MyOS { namespace Services { namespace Network { namespace DHCP {
	
using namespace Ethernet;

/// Implements the IDHCPClient interface
class DHCPClient : public IDHCPClient, public IP::IDatagramSocketClient {

public:
   DHCPClient() INITSECTION;

   virtual IIPEndpoint* createEndpoint(
      IInterface& driver,
      const char* clientId
   );

   virtual myos_result_t releaseAddress( IIPEndpoint& dhcpEndpoint );

   // IDatagramSocketClient
	virtual Thread*
      onRecv( IDatagramSocket& onSocket, const sockadr_t& from, const buffer& data );

private:
   // these could be parameters
   enum { MAX_WAIT_MS = 10000 };

   bool doDiscovery( const ethadr_t& localAddress, const char* clientId );
   bool doRequest( const DHCPPacket& offer );

   // Socket created to send&recv DHCP packets
   IP::IDatagramSocket* dhcpSocket;

   // client could create DHCP-enhanced classes implementing IIPEndpoint,
   // and maintain an array of at most N

   dhcpOptions_t options;
   const char* clientIdUsed;
   MultiThreading::t_syncpoint sync;
   bool gotReply;
};

}}}}   // namespace

#endif
