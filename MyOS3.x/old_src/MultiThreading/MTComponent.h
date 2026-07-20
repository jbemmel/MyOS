#ifndef MTComponent_H
#define MTComponent_H

#include "Core/IComponent.h"

#include "MultiThreading/IMultiThreadingImpl.h"

namespace MyOS {
namespace Timer {
class ITiming;
}
}

namespace MyOS {
namespace MultiThreading {

using MyOS::Core::IComponent;
using Timer::ITimerFacility;

class MTComponent : public IComponent {

	friend class Thread; // access to iKernelMemory
	
	IVirtualMemory* iVirtualMemory;
	ITimerFacility* iTiming;

public:
	inline static const char* const ID() { return "4b6c1c62-ce07-4242-996e-c47bf87eefd3"; }
	
	MTComponent(MyOS::Core::IContainer& container) throw() INITSECTION;
	virtual myos_result_t initialize(IContext& context, NVPAIR params[]) INITSECTION;
	virtual myos_result_t deinitialize(IContext& context);


	virtual myos_result_t queryInterface(IComponent& requestor,
			const uuid_t& uuid_t, IInterface*& result);

private:
	static MTComponent* instance;
	
public:
	inline static MTComponent& getInstance() {
		return *instance;
	}

private:

	friend class IMultiThreadingImpl; // access interfaces
	IMultiThreadingImpl impl1;
};

}
} // namespaces


#endif
