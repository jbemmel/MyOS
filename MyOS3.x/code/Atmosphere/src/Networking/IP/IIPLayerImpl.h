#ifndef IIPLayerImpl_H
#define IIPLayerImpl_H

#include "IIPLayer.h"

#include "IMultiThreading.h"	// for IPComponent
#include "IVirtualMemory.h"

namespace MyOS { namespace Networking { namespace IP {

   using Context::IContext;

class IIPLayerImpl : public IIPLayer 
{
public:
   IIPLayerImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( IContext& context, NVPAIR params[] ) INITSECTION;

   // Closes & destroys all endpoints
   void deinit( IContext& context );

   virtual IIPEndpoint* addBinding(
      IInterface& driver, ipadr_t localAdr, ipadr_t subnet );

   virtual ITCPSocket* createTCPSocket(
      ITCPSocketClient *client, ipadr_t binding=ipadr_t::ANY() );

   virtual ITCPServerSocket* createTCPServerSocket(
      ipport_t localport, ITCPServerSocketClient* client=0, ipadr_t binding=ipadr_t::ANY() );

   virtual IDatagramSocket* createUDPSocket(
      ipport_t localport=0, IDatagramSocketClient *client=0, ipadr_t binding=ipadr_t::ANY() );

   virtual IRawSocket* createRawSocket(
      E_IPPROTOCOL proto, IRawSocketClient *client=0, ipadr_t binding=ipadr_t::ANY() );

   virtual myos_result_t
      listBindings( IIPEndpoint** result, size_t max ) const;

   // Installs a TCP socket factory
   virtual myos_result_t installTCPSocketFactory( ITCPSocketFactory& factory );
   virtual myos_result_t removeTCPSocketFactory( ITCPSocketFactory& factory );

   // called by IPEndpoints
   void removeEndpoint( IIPEndpoint& endpoint );

   // called by IPEndpoints
   ITCPSocketFactory* getTCPFactory() const { return tcpfactory; }

private:
   bool findBinding( ipadr_t forAdr, IIPEndpoint*& result ) const;

   void printBindings( IO::OStream& out ) const;
   void printSockets( IO::OStream& out ) const;

   enum { MAXBINDINGS=5 };
   IIPEndpoint* bindings[ MAXBINDINGS ];

   ITCPSocketFactory* tcpfactory;
};
   
}}} // namespaces
#endif
