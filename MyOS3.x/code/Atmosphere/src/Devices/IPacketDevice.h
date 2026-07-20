#ifndef IPACKETDEVICE_H
#define IPACKETDEVICE_H

#include "IDevice.h"
#include "memtypes.h"
#include "Networking/Packet.h"

namespace MyOS {   
namespace Devices {

/***
 * MyOS support interface for (Ethernet) packet drivers
 * 
 * This is the API to be called by Ethernet NIC drivers
 */
class IPacketProcessor 
{
public:
    /**
     * Allocates a new packet buffer of at least the given size
     * 
     * @throws MM::OutOfMemoryException in case insufficient resources are available; This serves
     * as a flow control mechanism, the caller is expected to catch this exception and flush
     * the receive queue
     * 
     * @return newly allocated packet with a buffer of at least bytes 
     */
    virtual Networking::Packet& allocatePacket( size_t bytes ) 
        throw (MM::OutOfMemoryException) = 0;    

    /**
     * Allocates a new packet buffer, and fills it with the bytes read from DMA memory
     * Also calculates the IP level CRC checksum while copying
     * 
     * @return newly allocated packet filled with copied bytes and calculated CRC set
     * 
     * @note Could combine this with #onPacket, to avoid 2 calls
     */
    virtual Networking::Packet& allocateCopyCRCPacket( u8* dmabuf, size_t bytes ) 
        throw (MM::OutOfMemoryException) = 0;    
    
    
    /**
     * Callback into OS to process a received packet
     * 
     * @param packet - received packet, allocated via #allocatePacket
     * @param flags  - additional flags, such as multicast
     * 
     * @note After calling this method, the driver should no longer touch the packet
     */
    virtual void onPacket( Networking::Packet& packet, u32 flags ) = 0;
   
    virtual void onPacketSent( Networking::Packet& packet ) = 0;
    // virtual void onPacketAborted( const buffer& data, void* act ) = 0;
};


/// General interface for packet devices
/**
 * MyOS distinguishes three archetypes of devices by way of communication: 
 * 1. char devices   (communication using single characters)
 * 2. block  devices (communication using blocks (buffers) of data)
 * 3. packet devices (communication using packets (messages))
 * 
 * Packet devices differ subtly from block devices, packets can be considered as
 * blocks but each packet contains (device specific) addressing information
 * (especially destination)
 * 
 ** In fact, block devices could be abstracted as packet devices, using e.g.
 ** disk addresses as 'destination address'
 * 
 */
class IPacketDevice : public IDevice
{
   u32 mtu;		// Max. transmittable packetsize

protected:

	// TODO: Need more? sizeof(headers) ?
	inline IPacketDevice( Core::IComponent& c, VERSION v, 
	        const char* const u, const char* const n, u32 _mtu=0 ) 
	    : IDevice(c,v,u,n) { mtu=_mtu; }
   
public:
   /// @return This device's mtu, in bytes
   inline size_t getMaxPacketSize() const { return mtu; }

   /// Opens the device, and installs a reception handler
   /**
    * @param mode: Interpreted by the device
    */
   virtual myos_result_t open( IPacketProcessor& rx, u32 mode ) = 0;

   virtual void close( ) = 0;

   virtual myos_result_t sendPacket( const Networking::Packet& packet, u32 flags ) = 0;
};

}} // namespace

#endif
