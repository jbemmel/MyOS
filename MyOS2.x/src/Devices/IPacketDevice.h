#ifndef IPACKETDEVICE_H
#define IPACKETDEVICE_H

#include "IDevice.h"
#include "MM/memtypes.h"

namespace MyOS {   
namespace Devices {

// To be implemented by users of the PacketDevice class
class IPacketReceiver 
{
public:
	// TODO: Specify parameters in more detail ?
   virtual void onPacket( const buffer& data, u32 flags ) = 0;
};

// (optional) interface to be implemented by packet senders
class IPacketSender
{
public:
	// TODO: Specify parameters in more detail ?
   virtual void onPacketSent( const buffer& data, void* act ) = 0;
   virtual void onPacketAborted( const buffer& data, void* act ) = 0;
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
	inline IPacketDevice( Core::IComponent& c, VERSION v, u32 _mtu ) 
	: IDevice(c,v) { mtu=_mtu; }
   
public:
   /// @return This device's mtu, in bytes
   inline size_t getMaxPacketSize() const { return mtu; }

   /// Opens the device, and installs a reception handler
   /**
    * @param mode: Interpreted by the device
    */
   virtual myos_result_t open( IPacketReceiver& rx, u32 mode ) = 0;

   virtual void close( ) = 0;

   virtual myos_result_t sendPacket( const buffer& buf, u32 flags, IPacketSender* onDone, void* act ) = 0;
};

}} // namespace

#endif
