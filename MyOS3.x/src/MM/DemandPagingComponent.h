#ifndef DemandPagingComponent_H
#define DemandPagingComponent_H

#include "Core/IComponent.h"

#include "Paging/IDemandPagingImpl.h"

namespace MyOS {
namespace InterruptHandling {
class IInterruptHandling;
}
}

namespace MyOS {
namespace MM {
namespace Paging {

using MyOS::Core::IComponent;

class DemandPagingComponent : public IComponent {
public:
	// easier
	InterruptHandling::IInterruptHandling* iInterruptHandling;

public:
    inline static const char* const ID() { return "26954748-c804-4133-aa78-151c80f636da"; }

	DemandPagingComponent(MyOS::Core::IContainer& container) throw() INITSECTION;
	
	virtual myos_result_t initialize(Context::IContext& context,
			NVPAIR params[]) INITSECTION;
	
	virtual myos_result_t deinitialize(Context::IContext& context);

	virtual myos_result_t queryInterface(IComponent& requestor,
			const uuid_t& uuid, IInterface*& result);

private:
	static DemandPagingComponent* instance;
public:
	inline static DemandPagingComponent& getInstance() {
		return *instance;
	}

private:

	friend class IDemandPagingImpl; // access interfaces
	IDemandPagingImpl impl1;

};

}}} // namespaces


#endif
