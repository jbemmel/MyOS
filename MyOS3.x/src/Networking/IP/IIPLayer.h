#ifndef IIPLayer_H
#define IIPLayer_H
#include "Core/IInterface.h"

using MyOS::Core::IInterface;

#include "Core/IService.h"
#include "iptypes.h"

namespace MyOS { namespace Networking { namespace IP {

using Core::IInterface;

class IIPEndpoint;
class ITCPSocket;
class ITCPServerSocket;
class IDatagramSocket;
class IRawSocket;

// clients
class ITCPSocketClient;
class ITCPServerSocketClient;
class IDatagramSocketClient;
class IRawSocketClient;

class ITCPSocketFactory;

INTERFACE( IIPLayer, "77745694-3afa-42eb-81f2-167936737a67" )

   virtual myos_result_t registerService( Core::IService& svc );
   virtual myos_result_t removeService( Core::IService& svc );

   /**
    * Adds an IP binding
    *
    */
   virtual IIPEndpoint* addBinding(
        IInterface& driver, ipadr_t localAdr, ipadr_t subnet ) = 0;

   /**
    * @note The port is not a parameter, use a serversocket for that
    */
   virtual ITCPSocket* createTCPSocket(
        ITCPSocketClient *client, ipadr_t binding=ipadr_t::ANY() ) = 0;

    virtual ITCPServerSocket* createTCPServerSocket(
        ipport_t localport, ITCPServerSocketClient* client=0, ipadr_t binding=ipadr_t::ANY() ) = 0;

    virtual IDatagramSocket* createUDPSocket(
        ipport_t localport=0, IDatagramSocketClient *client=0, ipadr_t binding=ipadr_t::ANY() ) = 0;

    virtual IRawSocket* createRawSocket(
        E_IPPROTOCOL proto, IRawSocketClient *client=0, ipadr_t binding=ipadr_t::ANY() ) = 0;

   /**
    * Lists all current IP bindings up to max
    * @param result - array to receive bindings, must not be null
    * @param max    - max. number of bindings to return (size of array)
    * @return success ? (positive) number of valid bindings
    * @return E_MYOS_E_INVALIDARG if result==0 or max==0
    *
    * @note Each endpoint is able to report the nature of the physical
    *       interface through @see IIPEndpoint#getDriver
    */
   virtual myos_result_t
      listBindings( IIPEndpoint** result, size_t max ) const = 0;

   /**
    * Installs a TCP socket factory
    * This is a separate call, such that TCP can be implemented as an optional
    * component
    */
   virtual myos_result_t installTCPSocketFactory( ITCPSocketFactory& factory ) = 0;
   virtual myos_result_t removeTCPSocketFactory( ITCPSocketFactory& factory ) = 0;   
   
   protected: 
      Core::IService* serviceChain;	// @todo init to 0 in constructor!
      inline myos_result_t notifyServices( void* servicePoint ) {
         bool all = true;
         for( Core::IService* s = serviceChain; s; s = s->getNextInChain() ) {   
            all &= s->onServicePointCreated( servicePoint );
         }
         return all ? E_MYOS_SUCCESS : E_MYOS_E_FAIL;
      }
   


   };
   
}   
}} // namespaces
#endif
