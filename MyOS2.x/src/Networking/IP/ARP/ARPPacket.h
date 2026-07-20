#ifndef ARPPACKET_H
#define ARPPACKET_H

#include "types.h"
#include "Networking/Ethernet/EthernetPacket.h"		// ethadr_t

namespace MyOS { namespace Networking { namespace ARP {

using Ethernet::ethadr_t;

enum OPCODE {
   ARPOP_REQUEST   = 1,         /* ARP request   */
   ARPOP_REPLY     = 2,         /* ARP reply     */
   ARPOP_RREQUEST  = 3,         /* RARP request  */
   ARPOP_RREPLY    = 4,         /* RARP reply    */
   ARPOP_InREQUEST = 8,         /* InARP request */
   ARPOP_InREPLY   = 9,         /* InARP reply   */
   ARPOP_NAK       = 10         /* (ATM)ARP NAK  */
};

/// Format of an ARPPacket (on top of ethernet packet)
struct ARPPacket
{	
   nw16_t hwType;          // 1 or 6 for ethernet
   nw16_t protocol;        /// I am guessing this is EthernetProtocols.h
   u8 hwALength;
   u8 protocolALength;
   nw16_t opcode;

   /// variable length fields:
   /// hwSender, protocolSender   eg <ethadr_t, IP address>
   /// hwTarget, protocolTarget
   u8 varFields[20];

   u8* getSenderHwAdr() const       { return (u8*) varFields; }
   u8* getSenderProtocolAdr() const { return getSenderHwAdr()+hwALength; }
   u8* getTargetHwAdr() const       { return getSenderHwAdr()+hwALength+protocolALength; }
   u8* getTargetProtocolAdr() const { return getSenderHwAdr()+2*hwALength+protocolALength; }

   /// Only use these *after* hwALength & protocolALength have been set!
   /// for now, only ethernet
   void setSenderHwAdr( const u8* senderHw ) {
      memcpy( getSenderHwAdr(), senderHw, sizeof(ethadr_t) );
   }

   void setTargetHwAdr( const u8* targetHw ) {
      memcpy( getTargetHwAdr(), targetHw, sizeof(ethadr_t) );
   }

   /// For now, only IP...
   void setSenderProtocolAdr( ipadr_t ip ) {
      *((ipadr_t*) getSenderProtocolAdr() ) = ip;
   }

   void setTargetProtocolAdr( ipadr_t ip ) {
      *((ipadr_t*) getTargetProtocolAdr() ) = ip;
   }

   // To construct the prototype ARP request packet
   ARPPacket( ethadr_t local )
	   : hwALength(sizeof(ethadr_t)), protocolALength(sizeof(ipadr_t))
	{
		hwType   = ARP_ETHER;
		protocol = Ethernet::ETH_P_IP;
		opcode   = ARPOP_REQUEST;   		
   	
		memcpy( getSenderHwAdr(), local.bytes, sizeof(ethadr_t) );
		// memset( getTargetHwAdr(), 0, sizeof(ethadr_t) ); fill out later
		// *((ipadr_t*) getSenderProtocol() ) = localIP; done in sendRequest !
   }

} /* PACKED */;

}}}	// namespace

#endif
