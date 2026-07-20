#ifndef DHCPCLIENTSVCINSTANCE_H
#define DHCPCLIENTSVCINSTANCE_H

#include "Networking/IP/Sockets/IDatagramSocket.h"
#include "Networking/IP/IIPEndPoint.h"
#include "Devices/Network/IEthernetDevice.h"
#include "MultiThreading/IMultiThreading.h"
#include "MultiThreading/Thread.h"
#include "MultiThreading/Synchronization/CSyncPoint.h"

#include "DHCPPacket.h"

namespace MyOS { namespace Services { namespace Network { namespace DHCP {

using namespace Networking::IP;
using namespace Networking::Ethernet;

using Devices::Network::IEthernetDevice;
using MultiThreading::IRunnable;
using MultiThreading::Synchronization::CSyncPoint;
	
/// Maintains the per-endpoint DHCP client state
class DHCPClientSvcInstance : public IDatagramSocketClient, public IRunnable
{
   // these could be parameters
   enum { MAX_WAIT_US = 5000000 };
		
public:
	DHCPClientSvcInstance( IIPEndpoint& ep );
	void operator delete( void* );
	
	bool start( const char* clientId );

private:
	// @see IDatagramSocketClient
	virtual void onRecv( 
		IDatagramSocket& onSocket, const sockadr_t& from, const buffer& data );

	// @see IRunnable
	virtual int run( NVPAIR params[] );

	void runServiceThread( const ethadr_t& interfaceAdr, const char* clientId );
	bool doDiscovery( const ethadr_t& interfaceAdr, const char* clientId );
	bool doRequest( const DHCPPacket& offer );

	IIPEndpoint& endpoint;			// point of attachement
	IDatagramSocket* dhcpSocket;	// client socket, port E_DHCP_CLIENT_PORT	
	CSyncPoint sync;				// sync point for rx packets
	
	dhcpOptions_t options;			// received options
	const char* clientIdUsed;		// client id used, if any
	bool gotReply;
	
};
	
}}}}	// namespace

#endif
