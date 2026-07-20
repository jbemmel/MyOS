#ifndef MyOSCoreContainer_H
#define MyOSCoreContainer_H

#include "IComponent.h"

#include "MM/MMComponent.h"
#include "IH/IHComponent.h"
#include "Display/DisplayComponent.h"
#include "TimerFacility/TimerComponent.h"
#include "MT/MTComponent.h"
#include "MP/MPComponent.h"
#include "LD/LDComponent.h"
#include "Zip/ZipComponent.h"// TODO Move to Atmosphere

#include "../../Tests/TestComponent.h"

// #define TEST_IN_CORE

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
	InterruptHandling::IHComponent iH;
	Devices::Display::DisplayComponent display;
	Timer::TimerComponent timer;
	MultiThreading::MTComponent mT;
	MultiProcessing::MPComponent mP;
	DynamicLoading::LDComponent ld;
	Zip::ZipComponent zip;

#ifdef TEST_IN_CORE
	Tests::TestComponent test;
#endif
};
}
} // namespaces
#endif
