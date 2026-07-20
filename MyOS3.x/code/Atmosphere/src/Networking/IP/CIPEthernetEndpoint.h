/***************************************************************************
                          CIPEthernetEndPoint.h  -  description
                             -------------------
    begin                : Fri Jun 28 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef CIPETHERNETENDPOINT_H
#define CIPETHERNETENDPOINT_H

#include "iptypes.h"
#include "Devices/IPacketDevice.h"
#include "CIPEndpoint.h"
#include "ARP/AdrResolver.h"

namespace MyOS { namespace Networking { namespace IP {

using Util::HASHTABLE;
using namespace Ethernet;
using Devices::Network::IEthernetDevice;

class IIPLayerImpl;

/**
 * @note: If I ever add runtime replacement of the ethernet driver, remember
 *        to re-initialize the ARP cache!
 */
class CIPEthernetEndpoint : public CIPEndpoint, public Devices::IPacketProcessor
{
public:
	CIPEthernetEndpoint( IEthernetDevice& driver, ipadr_t local, ipadr_t subnet, IIPLayerImpl& l );

	inline void operator delete( void* ) {}
	virtual ~CIPEthernetEndpoint() {}

	// CIPEndpoint
	virtual void destroy();
	virtual IPPacket* createPacket( size_t maxDataSize );
	virtual void destroyPacket( IPPacket *p );
	virtual Devices::IPacketDevice& getDevice() const { return device; }

	// also this one, need to update ARP cache!
	virtual myos_result_t changeAddress( ipadr_t local, ipadr_t subnet );
	// virtual myos_result_t clearAddress();

   virtual myos_result_t setARPPolicy( u32 policyFlags ) ;
   virtual myos_result_t addARPEntry( ipadr_t forAdr, const buffer& hwAddress, u32 isStatic ) ;
   virtual myos_result_t removeARPEntry( ipadr_t forAdr ) ;
   virtual myos_result_t send( IPPacket& packet, ipadr_t to, size_t payload, E_IPPROTOCOL p, u32 flags );

   virtual myos_result_t setGateway( ipadr_t gateway );

   // IPacketProcessor
   virtual Networking::Packet& allocatePacket( size_t bytes ) 
           throw (MM::OutOfMemoryException);    
   virtual Networking::Packet& allocateCopyCRCPacket( u8* dmabuf, size_t bytes ) 
           throw (MM::OutOfMemoryException);    
   virtual void onPacket( Networking::Packet& packet, u32 flags );
   virtual void onPacketSent( Networking::Packet& packet );

private:
	myos_result_t sendIPOverEthernet( ipadr_t toAddress, EthernetPacket &p, size_t s, u32 flags );
	inline static EthernetPacket& toEth( IPPacket& p );

	ARP::AdrResolver arp;		// ethernet specific (?)
	IEthernetDevice& device;

	ipadr_t gateway;
	ipadr_t subnetBroadcast;	// calculated, cached
};

}}} // namespace

#endif
