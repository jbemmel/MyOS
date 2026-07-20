#ifndef ZipComponent_H
#define ZipComponent_H

#include "Core/IComponent.h"

#include "IZipUtilImpl.h"

namespace MyOS {
namespace Zip {

using MyOS::Core::IComponent;

class ZipComponent : public IComponent {
public:
	// easier
	IVirtualMemory* iVirtualMemory;

public:
	ZipComponent(MyOS::Core::IContainer& container) throw() INITSECTION;
	virtual myos_result_t initialize(Context::IContext& context, NVPAIR params[]) INITSECTION;
	virtual myos_result_t deinitialize(Context::IContext& context);

	virtual myos_result_t queryInterface(IComponent& requestor,
			const uuid_t& uuid, IInterface*& result);

private:
	static ZipComponent* instance;

public:
	inline static ZipComponent& getInstance() {
		return *instance;
	}

private:

	friend class IZipUtilImpl; // access interfaces
	IZipUtilImpl impl1;

};

}
} // namespaces


#endif
