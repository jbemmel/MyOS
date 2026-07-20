#ifndef IHComponent_H
#define IHComponent_H

#include "IComponent.h"
#include "IVirtualMemory.h"
#include "IH/IInterruptHandlingImpl.h"

namespace MyOS {
namespace InterruptHandling {

using MyOS::Core::IComponent;
using MM::IVirtualMemory;

/**
 * Component that provides the IInterruptHandling implementation
 *
 * @depend : none
 */
class IHComponent : public IComponent {

public:
    IVirtualMemory* iVirtualMemory;

	inline static const char* const ID() { return "ea21dd8e-ce8a-4777-8f03-5f0e39283c2c"; }

	IHComponent(MyOS::Core::IContainer& container) throw() INITSECTION;
	virtual myos_result_t initialize(Context::IContext& context,
			NVPAIR params[]) INITSECTION;
	virtual myos_result_t deinitialize(Context::IContext& context);

	virtual myos_result_t queryInterface(IComponent& requestor,
			const uuid_t& uuid, Core::IInterface*& result);

private:
	static IHComponent* instance;

public:
	inline static IHComponent& getInstance() {
		return *instance;
	}

private:

	friend class IInterruptHandlingImpl; // access interfaces
	IInterruptHandlingImpl impl1;

};

}
} // namespaces


#endif
