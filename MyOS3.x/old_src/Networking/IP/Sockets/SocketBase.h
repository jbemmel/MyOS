/**
 * The Socket design poses some specific problems:
 *
 * - The lowest layers need to treat all sockets uniformly when they forward data to the socket in onPacket
 * - All sockets need common data
 * - Avoid overhead of too many virtual function calls in packet processing path
 * - No multiple inheritance
 *
 * Therefore, I choose to derive all socket classes from a common abstract baseclass with some
 * (read-only for clients) data defined in it. Downside: implementation rather than a clean interface
 */
#ifndef SOCKETBASE_H
#define SOCKETBASE_H

#include "types.h"
#include "myosresult.h"
#include "../IIPEndPoint.h"

namespace MyOS {

class buffer;

namespace Networking { namespace IP {

class IPPacket;
class IIPEndpoint;

/// Options for open()
enum E_OPEN_OPTIONS {
   E_NONE = 0,
   E_ALLOW_MULTICAST = 1,
   E_ALLOW_BROADCAST = 2,
};

/**
 * The base class of all socket classes. Somewhat implementation-oriented for efficiency
 */
class CAbstractSocketBase
{
   friend class CIPLayer;
   friend class CIPEndpoint;
   
   // must not be called by clients
   virtual void onPacket( IPPacket& packet ) = 0;

protected:
	// disallowed, but GCC does not allow 'new' then either
	// void operator delete( void* );   // cannot be virtual, use destroy() instead

public:
   E_IPPROTOCOL getProtocol() const;
   ipadr_t getLocalAddress() const;
   ipadr_t getRemoteAddress() const;

   IIPEndpoint& getEndpoint() const;

   // only valid for UDP/TCP sockets
   ipport_t getLocalPort() const;
   ipport_t getRemotePort() const;

   /**
    * Gets the currently set TOS bits for all packets sent on this socket
    * @return bitmask of @see E_TOSBITS
    */
   u8 getTOSbits() const;

   /**
    * Sets the TOS bits applied to each packet sent on this socket
    * @param bits : OR'ed bits of @see E_TOSBITS
    * @return E_MYOS_SUCCESS if ok
    * @return E_MYOS_NOT_ALLOWED if application has insufficient permissions
    */
   myos_result_t setTOSbits( u8 bits );

   /**
    * Gets the TTL value set for all packets sent on this socket (default 0x40)
    * @return TTL value in hops
    */
   u8 getTTL() const;

   /**
    * Sets the TTL value applied to each packet sent on this socket
    * @param bits : OR'ed bits of @see E_TOSBITS
    * @return E_MYOS_SUCCESS if ok
    * @return E_MYOS_EINVAL if hops==0
    * @return E_MYOS_NOT_ALLOWED if application has insufficient permissions
    */
   myos_result_t setTTL( u8 hops );

   /**
    * Disables reception and transmission of further packets, until
    * the socket is re-opened or destroyed
    */
   virtual myos_result_t close() = 0;

   /**
    * Frees all resources used by this socket
    * This method can also be called by lower layers, for instance when the IPEndpoint
    * that this socket is connected to is removed or changes IPAddress
    */
   virtual myos_result_t destroy() = 0;   

#include "SocketBaseData.h"

}}} // namespace

#endif
