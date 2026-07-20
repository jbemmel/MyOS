/***************************************************************************
                          CTCPSocketFactory.h  -  description
                             -------------------
    begin                : Wed Oct 9 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef CTCPSOCKETFACTORY_H
#define CTCPSOCKETFACTORY_H

#include "ITCPSocketFactory.h"

namespace MyOS { namespace Networking { namespace IP { namespace TCP {
	
class CTCPSocketFactory : public ITCPSocketFactory
{
    virtual ITCPSocket* createTCPSocket( IIPEndpoint& endpoint,
      ITCPSocketClient *client );
    virtual ITCPServerSocket* createTCPServerSocket(
      IIPEndpoint& ep, ipport_t localport, ITCPServerSocketClient* client );
      
public:
	void operator delete( void* );      
};

}}}} // namespace

#endif
