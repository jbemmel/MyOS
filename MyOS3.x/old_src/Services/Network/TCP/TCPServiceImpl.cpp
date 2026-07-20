#include "Services/Network/TCP/TCPService.h"
#include "Networking/IP/IIPLayer.h"
#include "Networking/IP/Sockets/TCP/CTCPSocketFactory.h"

namespace MyOS { namespace Services { namespace Network { namespace TCP {

using Networking::IP::IIPEndpoint;
using Networking::IP::TCP::CTCPSocketFactory;

// virtual 
bool 
TCPService::onServicePointCreated( void* servicePoint )
{
	// install new factory in the IIPLayer, XX only needed once
	IIPEndpoint* endpoint = (IIPEndpoint*) servicePoint;
	DPRINTK( "\nTCP onServicePointCreated: %x", endpoint );
	static bool installed = false;
	if (!installed) {
		installed = true;
		CTCPSocketFactory* fac = new CTCPSocketFactory();
		iIPLayer->installTCPSocketFactory( *fac );
	}
	return true;
}


}}}}	// namespace
