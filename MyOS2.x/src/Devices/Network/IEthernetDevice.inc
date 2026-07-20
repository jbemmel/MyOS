#ifdef INCLUDE_PHASE

#include "Networking/Ethernet/EthernetPacket.h"

namespace MyOS { namespace Devices { namespace Network {
	class IEthernetDevice;
	
	using Networking::Ethernet::ethadr_t;
}}}

namespace MyOS { namespace Drivers { namespace Network {
	using MyOS::Devices::Network::IEthernetDevice;
}}}

#else

public:
	const ethadr_t& getLocalAddress() const { return localAddress; }
		
	// additional common datamembers
	ethadr_t localAddress;

	// additional result codes
	enum E_RESULT_EXT {
		LINK_DOWN = E_MYOS_ERROR | 0x20000,
	};

#endif
