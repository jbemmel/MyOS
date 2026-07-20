#ifndef ASContainer_H
#define ASContainer_H

#include "IComponent.h"
#include "IMultiThreading.h"

#include "../Repository/RepositoryComponent.h"
#include "../Devices/DriverManagerComponent.h"
#include "../Networking/IP/IPComponent.h"
#include "../Devices/Network/NE2000/NE2000Driver.h"
#include "../Services/Network/TCP/TCPService.h"
#include "../Services/Network/DHCP/DHCPClientService.h"

#include "../../Tests/TestComponent.h"

namespace MyOS {
namespace Atmosphere {

/**
 * Container for the Atmosphere subsystems.
 * 
 * Derived from IRunnable because it runs as a separate process
 */ 
class ASContainer : public MyOS::Core::IContainer, public MultiThreading::IRunnable {
public:
    ASContainer() INITSECTION;
	virtual myos_result_t initialize( Context::IContext& context, NVPAIR params[]) INITSECTION;
	virtual myos_result_t deinitialize( Context::IContext& context);

	// IComponent
	virtual myos_result_t queryInterface(IComponent& requestor,
			const uuid_t& uuid, IInterface*& result);

	// IContainer
	virtual myos_result_t queryComponent(IComponent& requestor,
			const uuid_t& uuid, IComponent*& result);

	virtual myos_result_t destroyComponent(IComponent& c);
	
	/**
	 * MultiThreading::IRunnable
	 * 
	 * This method will call this->initialize
	 */
	virtual int run( NVPAIR params[] );
	virtual void handleSignal( u32 code, void* data ) throw() {};

private:
	static ASContainer* instance;

public:
	inline static ASContainer& getInstance() {
		return *instance;
	}

private:
	Context::RepositoryComponent rc;

	Drivers::DriverManagerComponent driverManager;

	// Networking
	Drivers::Network::NE2000::NE2000Driver ne2000;
	Networking::IP::IPComponent ip;
	Services::Network::TCP::TCPService tcp;
	Services::Network::DHCP::DHCPClientService dhcp;

	Tests::TestComponent test;
};
}
} // namespaces
#endif
