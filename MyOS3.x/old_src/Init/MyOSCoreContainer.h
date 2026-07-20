#ifndef MyOSCoreContainer_H
#define MyOSCoreContainer_H

#include "Core/IComponent.h"

#include "MM/MMComponent.h"
#include "Context/RepositoryComponent.h"
#include "InterruptHandling/IHComponent.h"
#include "MM/DemandPagingComponent.h"
#include "Devices/Display/DisplayComponent.h"
#include "TimerFacility/TimerComponent.h"
#include "MultiThreading/MTComponent.h"
#include "MultiProcessing/MPComponent.h"
#include "Drivers/DriverManagerComponent.h"
#include "Networking/IP/IPComponent.h"
#include "Tests/TestComponent.h"

#include "Drivers/Network/NE2000/NE2000Driver.h"
#include "Drivers/Keyboard/Standard/KeyboardDriver.h"
#include "Services/Network/TCP/TCPService.h"

#include "BIOS/BIOSComponent.h"

namespace MyOS {
namespace Init {

class MyOSCoreContainer : public MyOS::Core::IContainer {
public:
	MyOSCoreContainer() INITSECTION;
	virtual myos_result_t initialize( Context::IContext& context, NVPAIR params[]) INITSECTION;
	virtual myos_result_t deinitialize( Context::IContext& context);

	// IComponent
	virtual myos_result_t queryInterface(IComponent& requestor,
			const uuid_t& uuid, IInterface*& result);

	// IContainer
	virtual myos_result_t queryComponent(IComponent& requestor,
			const uuid_t& uuid, IComponent*& result);

	virtual myos_result_t destroyComponent(IComponent& c);

private:
	static MyOSCoreContainer* instance;

public:
	inline static MyOSCoreContainer& getInstance() {
		return *instance;
	}

private:
	MM::MMComponent mM;
	Context::RepositoryComponent repository;
	InterruptHandling::IHComponent iH;
	MM::Paging::DemandPagingComponent demandPaging;
	Devices::Display::DisplayComponent display;
	Timer::TimerComponent timer;
	MultiThreading::MTComponent mT;
	MultiProcessing::MPComponent mP;
	Drivers::DriverManagerComponent driverManager;
	Drivers::Network::NE2000::NE2000Driver nE2000;
	Drivers::Keyboard::Standard::KeyboardDriver kb;
	Networking::IP::IPComponent iP;
	Services::Network::TCP::TCPService tCP;
	
	BIOS::BIOSComponent bios;
	
	Tests::TestComponent test;
};
}
} // namespaces
#endif
