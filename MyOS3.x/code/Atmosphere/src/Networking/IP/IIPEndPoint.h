/***************************************************************************
IPEndPoint.h  -  description
-------------------
begin                : Fri Jun 28 2002
copyright            : (C) 2002 by Jeroen van Bemmel
email                : j.vanbemmel@home.nl
***************************************************************************/
#ifndef IPENDPOINT_H
#define IPENDPOINT_H

#include "memtypes.h"
#include "iptypes.h"
#include "myosresult.h"

namespace MyOS {

class buffer;

namespace Core { class IInterface; }
namespace IO { class OStream;}
namespace Devices { class IPacketDevice; }

namespace Networking { namespace IP {

   using MM::linadr_t;

   class IPPacket;
   class ITCPSocket;
   class ITCPServerSocket;
   class IDatagramSocket;
   class IRawSocket;
   class ITCPSocketClient;
   class ITCPServerSocketClient;
   class IDatagramSocketClient;
   class IRawSocketClient;
   class CAbstractSocketBase;

   enum E_ARP_POLICY_FLAGS {
      E_ARP_NOREPLY       = 0x00,  ///< Disables generation of ARP replies
      E_ARP_REPLY         = 0x01,  ///< If this endpoint should reply to ARP requests, default true
      E_ARP_PROXY         = 0x02,  ///< If this endpoint should be a proxy (i.e. reply to all)
      E_ARP_UNSOLLICITED  = 0x04,  ///< If this endpoint should accept unsollicited replies, default false
   };

	/// Options for send
	enum E_SEND_OPTIONS {
		E_SEND_NONE				= 0,
		E_SEND_PACKET_REUSED 	= 1,	// When reusing a packet instead of allocate
	};


   /**
   * The IP (layer 3) interface, responsible for implementing the IP, UDP, 
   * TCP protocols and more raw protocols
   *
   *  - fragmentation and re-assembly
   *  - IP address management
   */
   class IIPEndpoint
   {
	public:
      virtual void destroy() = 0;

      /**
       * Creates a packet suitable for sending on this endpoint
       */
      virtual IPPacket* createPacket( size_t maxDataSize ) = 0;

      /**
       * Destroys a packet previously created with createPacket()
       */
      virtual void destroyPacket( IPPacket *p ) = 0;

      /// Sends an IP packet to its destination
      /**
       * @param flags can be E_SEND_PACKET_REUSED to prevent packet deallocation
       * 
       * The packet may be subjected to buffering at lower layers, so upon return from this function
       * it may or may not already have been sent
       * 
       * TODO: RFC1122 mandates that 5 TOS bits should also be passed
       * Merge p+flags+tos in 1 32-bit parameter
       * 
       * TODO Could add a hint for ethernet hardware address, if not an ARP query may follow
       * Useful for replies to requests (e.g. ICMP PING) 
       */
      virtual myos_result_t send( IPPacket& packet, ipadr_t to, size_t payload, 
              E_IPPROTOCOL p, u32 flags ) = 0;

      /**
       * Changes the IP address & subnetmask of this endpoint
       * @param local  - New IP address for thie endpoint
       * @param subnet - New subnetmask for this endpoint, if 0 the existing value is kept
       *
       * @return E_MYOS_SUCCESS if ok
       * @return E_MYOS_E_INVALIDARG if local is not a valid endpoint IP adr
       */
      virtual myos_result_t changeAddress( ipadr_t local, ipadr_t subnet ) = 0;

	  /**
	   * Clear the currently assigned IP address (also flushes ARP cache)
	   */
	  virtual myos_result_t clearAddress() = 0;

      /**
       * Sets ARP options
       * @param policyFlags : Bitmask of E_ARP_POLICY flag bits
       * @return E_MYOS_SUCCESS iff ok
       */
      virtual myos_result_t setARPPolicy( u32 policyFlags ) = 0;

      /**
       * Adds an ARP address entry to the ARP cache
       * @param forAdr - the IP address to add
       * @param hwAddress - the hardware address to add, currently only ethernet
       * @param isStatic  - true iff entry should be static, default false
       * @return E_MYOS_SUCCESS iff ok
       */
      virtual myos_result_t addARPEntry( ipadr_t forAdr, 
		  const buffer& hwAddress, u32 isStatic=false ) = 0;
   
      /**
       * Removes an ARP entry from the cache
       * @param forAdr - the IP address to remove
       */
      virtual myos_result_t removeARPEntry( ipadr_t forAdr ) = 0;

      /// Socket management: socket is removed when destroyed
      virtual ITCPSocket* createTCPSocket( ITCPSocketClient *client ) = 0;

      virtual ITCPServerSocket* createTCPServerSocket(
         ipport_t localport, ITCPServerSocketClient* client ) = 0;

      /**
       * Creates an UDP socket on this endpoint
       * @note Creating a socket does *not* open() it yet!
       */
      virtual IDatagramSocket* createUDPSocket(
         IDatagramSocketClient *client, ipport_t localport=0 ) = 0;

      virtual IRawSocket* createRawSocket(
         E_IPPROTOCOL proto, IRawSocketClient *client ) = 0;

      /**
       * Sets the IP address of a gateway
       *
       * The gateway is used to lookup a network address for packets destined
       * outside the local network (an implicit default route)
       *
       * @param gateway : IP address of the gateway
       * 
       * @return E_MYOS_SUCCESS if set
       * @return E_MYOS_FAIL if not supported
       * @return E_MYOS_NOTIMPL if not implemented
       */
      virtual myos_result_t setGateway( ipadr_t gateway ) = 0;

      /**
       * Provides access to the underlying device interface
       */
      virtual Devices::IPacketDevice& getDevice() const = 0;

      // for IDL, could be removed
      virtual void listSockets( IO::OStream& out ) const = 0;

      class Client {
         public:
         // will receive all IP packets that are not handled by this endpoint
         // @todo Other protocols such as ARP?
         virtual void onPacket( IPPacket& packet, size_t payload ) = 0;
      };

      // non-virtual methods
      void bindClient( Client& cl )   { client=&cl; }
      void removeClient()             { client=0; }

      ipadr_t getLocalAddress() const { return localAddress; }
      ipadr_t getSubnet() const       { return subnet; }

      // Used internally by socket implementation
      virtual myos_result_t addSocket( CAbstractSocketBase& sock ) = 0;
      virtual myos_result_t removeSocket( CAbstractSocketBase& sock ) = 0;

   protected:
      // constructor, called by subclass
      IIPEndpoint( ipadr_t localAdr, ipadr_t subnetMask )
         : localAddress(localAdr), subnet(subnetMask) {}

      ipadr_t localAddress;
      ipadr_t subnet;
      Client* client;	   // optional
   };

}}} // namespace

#endif
