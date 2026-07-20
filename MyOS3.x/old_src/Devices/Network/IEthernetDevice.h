#ifndef IEthernetDevice_H
#define IEthernetDevice_H

#include "Devices/IPacketDevice.h"
#include "Networking/Ethernet/EthernetPacket.h"

namespace MyOS
{
namespace Devices
{
namespace Network
{

using Networking::Ethernet::ethadr_t;

SUB_INTERFACE( IEthernetDevice, IPacketDevice,
        "5f119878-41d6-4374-bdb5-543a2c90ef21" )

virtual myos_result_t getAddress(
        IO::OStream& out
) const = 0;

    const ethadr_t& getLocalAddress() const { return localAddress; }

    // additional result codes
    enum E_RESULT_EXT {
        LINK_DOWN = E_MYOS_ERROR | 0x20000,
    };
        
protected:    
    // additional common datamembers
    ethadr_t localAddress;
};

}
}
} // namespaces
#endif
