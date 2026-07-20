#ifndef ITCPSOCKETFACTORY_H
#define ITCPSOCKETFACTORY_H

#include "iptypes.h"

namespace MyOS { namespace Networking { namespace IP {

class IIPEndpoint;
class ITCPSocket;
class ITCPSocketClient;
class ITCPServerSocket;
class ITCPServerSocketClient;

/// Separate factory interface to allow independent TCP implementation
/**
 * All methods in this factory are identical to the IPLayer variants
 * This could be generalized to all kinds of sockets
 */
class ITCPSocketFactory
{
public:
    virtual ITCPSocket* createTCPSocket( IIPEndpoint& endpoint,
      ITCPSocketClient *client ) = 0;
    virtual ITCPServerSocket* createTCPServerSocket(
      IIPEndpoint& ep, ipport_t localport, ITCPServerSocketClient* client ) = 0;
};

}}}	// namespace

#endif
