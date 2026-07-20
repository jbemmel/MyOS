#include "IIPLayerImpl.h"
#include "Networking/IP/IPComponent.h"
#include "IService.h"

// supported bindings
#include "CIPEthernetEndpoint.h"

namespace MyOS { namespace Networking { namespace IP {

IIPLayerImpl::IIPLayerImpl( MyOS::Core::IComponent& c )
: IIPLayer( c, VERSION(1,0) )
{
	// needed! Memory is not clear
	memset_aligned( bindings, 0, sizeof(bindings) );
}

bool 
IIPLayerImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
    // IIPLayer constructor not available, so...
    serviceChain = 0;
    
	return true;
}

void 
IIPLayerImpl::deinit( IContext& context )
{
   for (u32 b=0; b<MAXBINDINGS; ++b) {
      if (IIPEndpoint* bnd = bindings[b]) {
         bindings[b]=0;
         bnd->destroy();
      }
   }
}

// virtual
IIPEndpoint*
IIPLayerImpl::addBinding(
   IInterface& driver, ipadr_t localAdr, ipadr_t subnet )
{
	// Union of all supported devices, currently 1
	union {
		IEthernetDevice* eth;
	} u;

   // note: Taking a shortcut here, it is possible that not all duplicates
   // are caught (addBinding 2x, remove first, then add duplicate will work)
   // TODO: Use hashtable or something to prevent this
   for (u32 b=0; b<MAXBINDINGS; ++b) {
      if (bindings[b]==0) {
         if ((u.eth = driver.castTo<IEthernetDevice>())) {
         	IIPEndpoint* ep = new CIPEthernetEndpoint( *u.eth, localAdr, subnet, *this );
            bindings[b] = ep;
            
            // ADDED: notify registered services, if any
            notifyServices( ep );
            
			return ep;
         } else {
            // unsupported driver class
            DPRINTK( "\n##ERROR: Unknown driver class!" );
            return 0;
         }
      } else if (bindings[b]->getLocalAddress()==localAdr) {
         DPRINTK( "\n##ERROR: Duplicate IPEndpoint!" );
         return 0;   // duplicate
      }
   }
   DPRINTK( "\n##ERROR: No free bindingpoints!" );
   return 0;
}

void
IIPLayerImpl::removeEndpoint( IIPEndpoint& endpoint )
{
   for (u32 b=0; b<MAXBINDINGS; ++b) {
      if (bindings[b]==&endpoint) {
         bindings[b]=0;
         return;
      }
   }
}

// virtual
ITCPSocket*
IIPLayerImpl::createTCPSocket( ITCPSocketClient *client, ipadr_t ipadr )
{
   IIPEndpoint* b;
   return findBinding(ipadr,b) ? b->createTCPSocket(client) : 0;
}

// virtual
ITCPServerSocket*
IIPLayerImpl::createTCPServerSocket(
   ipport_t localport, ITCPServerSocketClient* client, ipadr_t ipadr )
{
   IIPEndpoint* b;
   return findBinding(ipadr,b) ? b->createTCPServerSocket(localport,client) : 0;
}

// virtual
IDatagramSocket*
IIPLayerImpl::createUDPSocket(
   ipport_t localport, IDatagramSocketClient *client, ipadr_t ipadr )
{
   IIPEndpoint* b;
   return findBinding(ipadr,b) ? b->createUDPSocket(client,localport) : 0;
}

// virtual
IRawSocket*
IIPLayerImpl::createRawSocket( E_IPPROTOCOL proto, IRawSocketClient *client, ipadr_t ipadr )
{
   IIPEndpoint* b;
   return findBinding(ipadr,b) ? b->createRawSocket(proto,client) : 0;
}

// virtual
myos_result_t
IIPLayerImpl::listBindings( IIPEndpoint** result, size_t max ) const
{
   if (result==0 || max==0) return E_MYOS_E_INVALIDARG;

   size_t count=0;
   for (u32 b=0; b < min(max,MAXBINDINGS); ++b)
   {
      if (bindings[b]) {
         result[count++] = bindings[b];
      }
   }
   return (myos_result_t) count;
}

bool
IIPLayerImpl::findBinding( ipadr_t forAdr, IIPEndpoint*& result ) const
{
   // could use hashtable instead when needed
   for (u32 b=0; b < MAXBINDINGS; ++b)
   {
      if (IIPEndpoint* p=bindings[b]) {

         if (p->getLocalAddress()==forAdr) { result=p; return true; }
      } else return false;
   }
   return false;
}

// perhaps make optional
void
IIPLayerImpl::printBindings( IO::OStream& out ) const
{
   // could use hashtable instead when needed
   out.printf( "<bindings>" );
   for (u32 b=0; b < MAXBINDINGS; ++b)
   {
      if (IIPEndpoint* p=bindings[b]) {
         ipadr_t ip = p->getLocalAddress();
         ipadr_t sn = p->getSubnet();
         out.printf( "\n<binding ip=\"%u.%u.%u.%u\" subnet=\"%u.%u.%u.%u\"/>",
            ip[0],ip[1],ip[2],ip[3],sn[0],sn[1],sn[2],sn[3] );
         // could add gateway
      }
   }
   out.printf( "\n</bindings>" );
}

// perhaps make optional
void

IIPLayerImpl::printSockets( IO::OStream& out ) const
{
   // could use hashtable instead when needed
   out.printf( "<sockets>" );
   for (u32 b=0; b < MAXBINDINGS; ++b)
   {
      if (IIPEndpoint* p=bindings[b]) {
         p->listSockets(out);
      }
   }
   out.printf( "\n</sockets>" );
}

// virtual
myos_result_t
IIPLayerImpl::installTCPSocketFactory( ITCPSocketFactory& factory )
{
   tcpfactory = &factory;  // overwrite
   return E_MYOS_SUCCESS;
}

// virtual
myos_result_t
IIPLayerImpl::removeTCPSocketFactory( ITCPSocketFactory& factory )
{
   if (tcpfactory == &factory) {
      tcpfactory = 0;
      return E_MYOS_SUCCESS;
   }
   return E_MYOS_E_FAIL;
}

}}}  // namespaces
