/***************************************************************************
                          DHCPPacket.cpp  -  description
                             -------------------
    begin                : Tue Sep 10 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#include "DHCPPacket.h"
#include "Services/Network/DHCP/DHCPClientService.h"
#include "Timer/ITiming.h"
#include "string.h"


// debug
// #define DHCP_VERBOSE

#ifndef DHCP_VERBOSE
#undef DPRINTK
#define DPRINTK(X,args...)
#endif

namespace MyOS { namespace Services { namespace Network { namespace DHCP {

// Some (not all) DHCP options (RFC 2132)
enum E_DHCP_OPTIONS {
   E_DHCP_OPTION_PAD          = 0,
   E_DHCP_OPTION_SUBNETMASK   = 1,
   E_DHCP_OPTION_TIMEOFFSET   = 2,
   E_DHCP_OPTION_ROUTER       = 3,
   E_DHCP_OPTION_NAMESERVERS  = 5,  // IEN 116, whatever that may be
   E_DHCP_OPTION_DNSSERVERS   = 6,
   E_DHCP_OPTION_HOSTNAME     = 12,
   E_DHCP_OPTION_DOMAINNAME   = 15,
   E_DHCP_OPTION_REQUESTED_IP = 50, // only in DHCP request by client
   E_DHCP_OPTION_IPLEASETIME  = 51,
   E_DHCP_OPTION_DHCPTYPE     = 53,
   E_DHCP_OPTION_SERVERID     = 54,
   E_DHCP_OPTION_ERRMESSAGE   = 56,
   E_DHCP_OPTION_RENEWTIME    = 58,
   E_DHCP_OPTION_REBINDTIME   = 59,
   E_DHCP_OPTION_CLIENTID     = 61, // followed by len, type

   // see http://www.isi.edu/in-notes/iana/assignments/bootp-dhcp-extensions/
   // e.g. E_DHCP_OPTION_IPX_OPTIONS  = 110,

   E_DHCP_OPTION_END          = 255,
};

/**
 * Creates a dhcp request packet for ethernet
 */
DHCPPacket::DHCPPacket( const ethadr_t& localAddress, E_DHCP_MSGTYPE_VALUES t )
: opcode(E_BOOT_REQUEST), hwAddressType(Networking::ARP::ARP_ETHER)
      , hwAddressLen( sizeof(ethadr_t) ), hwOptions(0)
{
	flags = 0;
	
	// There is unclarity about this, seconds since boot or since transmission
	secondsSinceBoot = DHCPClientService::getInstance().iTiming->now(Timer::seconds);

	// must be "random", last one sent remembered in send()
	transactionId = secondsSinceBoot.netorder() + (u32) this;

	// since the packet might be constructed on stack, some fields may be dirty
	memset_aligned( clientHwAddress, 0, 16+64+128 );
	*((ethadr_t*) clientHwAddress) = localAddress;

	// magicOption field used internally as ptr into options first, until wrapup
	magicOptionValue = (u32) (&optionbytes);

	// fill in DHCP type
	setDHCPType(t);
}

DHCPPacket::DHCPPacket( const DHCPPacket& offer, E_DHCP_MSGTYPE_VALUES type )
{
   new (this) DHCPPacket( *((ethadr_t*) &offer.clientHwAddress), type );

   // RFC 2131: MUST use same value for secs
   secondsSinceBoot = offer.secondsSinceBoot;

   // RFC 2131: xid must be copied from server
   transactionId = offer.transactionId;
}

void
DHCPPacket::setDHCPType( E_DHCP_MSGTYPE_VALUES t )
{
   u8* options = (u8*) magicOptionValue;
   *options++ = E_DHCP_OPTION_DHCPTYPE;   // tag
   *options++ = 1;                        // len
   *options++ = t;                        // type
   magicOptionValue += 3;
}

void
DHCPPacket::addClientIdOption( const char* id )
{
   u8 optionLength = strlen(id)+1;
   u8* options = (u8*) magicOptionValue;
   *options++ = E_DHCP_OPTION_CLIENTID;   // tag
   *options++ = optionLength;             // len
   *options++ = 0;                        // type, eg ARP hw or 0 for fully q adr
   strcpy( (char*) options, id );
   magicOptionValue += optionLength + 2;
}

void
DHCPPacket::addServerIdOption( ipadr_t id )
{
   u8* options = (u8*) magicOptionValue;
   *options++ = E_DHCP_OPTION_SERVERID;   // tag
   *options++ = sizeof(id);               // len
   *((ipadr_t*)options) = id;
   magicOptionValue += sizeof(id) + 2;
}

void
DHCPPacket::addRequestedIPOption( ipadr_t ip )
{
   u8* options = (u8*) magicOptionValue;
   *options++ = E_DHCP_OPTION_REQUESTED_IP;  // tag
   *options++ = sizeof(ip);                  // len
   *((ipadr_t*)options) = ip;
   magicOptionValue += sizeof(ip) + 2;
}

size_t
DHCPPacket::wrapUp()
{
	// pad options, add end
	u8* options = (u8*) magicOptionValue;
	while ((((u32)options)&3)!=3) *options++ = E_DHCP_OPTION_PAD;
	*options++ = E_DHCP_OPTION_END;

	ASSERTION( options < optionbytes+sizeof(optionbytes), E_CRITICAL );

	// set magic to correct value
	magicOptionValue = E_MAGIC_OPTION_COOKIE_LE;

	return (size_t) (options - (u8*) this);
}

bool
DHCPPacket::parseOptions( const u8* packetLimit, dhcpOptions_t& results ) const
{
   // clear type in options
   results.packetType = E_DHCP_INVALID;

   // verify
   if (magicOptionValue != E_MAGIC_OPTION_COOKIE_LE) return false;

   // set assigned ip to value in this packet
   results.assignedIP = yourClientIp;

   u8* options = (u8*) (&magicOptionValue + 1);
   while (*options && options < packetLimit)
   {
      switch (*options++)
      {
      case E_DHCP_OPTION_PAD:
         break;   // ignore

      case E_DHCP_OPTION_SUBNETMASK:
      {
         results.subnetMask = ipadr_t( *((u32*) (options+1)), true );
         DPRINTK( "\nDHCP subnetmask: %x", results.subnetMask.host() );
         break;
      }

      case E_DHCP_OPTION_TIMEOFFSET:
         DPRINTK( "\nDHCP timeoffset: %x len=%X",
            htons32( *((u32*) (options+1)) ), *options );
         break;

      case E_DHCP_OPTION_ROUTER:
         results.router = ipadr_t( *((u32*) (options+1)), true );
         DPRINTK( "\nRouter: %x len=%X", results.router.host(), *options );
         break;

      case E_DHCP_OPTION_DHCPTYPE:
         results.packetType = (E_DHCP_MSGTYPE_VALUES) *(options+1);
         DPRINTK( "\nDHCP msgType = %X len=%X", results.packetType, *options );
         break;

      case E_DHCP_OPTION_DNSSERVERS:   // can be more than 1
         results.primaryDNSServer = ipadr_t( *((u32*) (options+1)), true );
         DPRINTK( "\nDHCP primaryDNSServer = %x", results.primaryDNSServer.host() );
         if (*options>4) {
            results.backupDNSServer = ipadr_t( *((u32*) (options+5)), true );
            DPRINTK( "\nDHCP backupDNSServer = %x", results.backupDNSServer.host() );
         }
         break;

      case E_DHCP_OPTION_HOSTNAME:
         ASSERTIONv( *options < E_DHCP_MAX_HOSTNAME_LEN, E_ERROR, *options );
         strncpy(
            (char*) results.hostname,
            (const char*) (options+1),
            E_DHCP_MAX_HOSTNAME_LEN+1 );
         DPRINTK( "\nHostname: %s", options+1 );
         break;

      case E_DHCP_OPTION_DOMAINNAME:
         ASSERTIONv( *options < E_DHCP_MAX_DOMAIN_LEN, E_ERROR, *options );
         strncpy(
            (char*) results.domainname,
            (const char*) (options+1),
            E_DHCP_MAX_DOMAIN_LEN+1 );
         DPRINTK( "\nDomainname: %s", options+1 );
         break;

      case E_DHCP_OPTION_SERVERID:
         results.serverIp = ipadr_t( *((u32*) (options+1)), true );
         DPRINTK( "\nServerID val=%x", results.serverIp.host() );
         break;

      case E_DHCP_OPTION_ERRMESSAGE:
         DPRINTK( "\nDHCP error message=%s", options+1 );
         break;

      case E_DHCP_OPTION_IPLEASETIME:
         results.leaseTime = htons32( *((u32*)(options+1)) );
         DPRINTK( "\nIPAddress leasetime: %x", results.leaseTime );
         break;

      case E_DHCP_OPTION_RENEWTIME:
         results.renewTime = htons32( *((u32*)(options+1)) );
         DPRINTK( "\nIPAddress renewTime: %x", results.renewTime );
         break;

      case E_DHCP_OPTION_REBINDTIME:
         results.rebindTime = htons32( *((u32*)(options+1)) );
         DPRINTK( "\nIPAddress rebindTime: %x", results.rebindTime );
         break;

      case E_DHCP_OPTION_END:
         return true;

      default:
         DPRINTK( "\nUnknown DHCP option tag: %u len=%X", options[-1], *options );
      }

      // always advance
      options += *options + 1;
   }

   DPRINTK( "\nWent beyond DHCP packet!" );
   return false;
}

}}}} // namespace
