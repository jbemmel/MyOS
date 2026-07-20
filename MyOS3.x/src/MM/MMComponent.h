#ifndef MMComponent_H
#define MMComponent_H

#include "Core/IComponent.h"

#include "Virtual/IVirtualMemoryImpl.h"

#include "Physical/IPhysicalMemoryImpl.h"

namespace MyOS {
namespace MM {

using MyOS::Core::IComponent;

class MMComponent : public IComponent {

public:
    inline static const char* const ID() { return "b4e0fdc1-c04a-4149-baaf-037bd91ed349"; }
	
	MMComponent(MyOS::Core::IContainer& container) throw() INITSECTION;
	
	virtual myos_result_t initialize(Context::IContext& context,
			NVPAIR params[]) INITSECTION;
	virtual myos_result_t deinitialize(Context::IContext& context);

	virtual myos_result_t queryInterface(IComponent& requestor,
			const uuid_t& uuid, IInterface*& result);

private:
	static MMComponent* instance;
	
public:
	inline static MMComponent& getInstance() {
		return *instance;
	}

private:

	friend class IVirtualMemoryImpl; // access interfaces
	IVirtualMemoryImpl impl1;

	friend class IPhysicalMemoryImpl; // access interfaces
	friend class PBT;
	IPhysicalMemoryImpl impl2;

	friend class MultiThreading::Thread;
	inline void freeInitPages() { impl2.freeInitPages(); }
};

}
} // namespaces


#endif
