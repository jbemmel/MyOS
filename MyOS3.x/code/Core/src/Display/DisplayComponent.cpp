#include "IContext.h"
#include "DisplayComponent.h"

namespace MyOS {
namespace Devices {
namespace Display {

//const uuid_t DisplayComponent::ID = "c706b2e4-5f59-11d6-946d-0010a708e02d";
DisplayComponent* DisplayComponent::instance INITINSTANCE;

DisplayComponent::DisplayComponent(MyOS::Core::IContainer& container) throw() :
	IComponent(container, VERSION(1, 1), ID(), (IComponent*&) instance )
	, impl1(*this)
{
}

myos_result_t DisplayComponent::initialize(Context::IContext& context, NVPAIR params[]) {

	if (!impl1.init(context, params))
		return E_MYOS_E_FAIL;

	if (context.add(impl1)!=E_MYOS_SUCCESS)
		return E_MYOS_E_REGFAILED;

	return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t DisplayComponent::deinitialize(Context::IContext& context) {

	context.remove(impl1 );

	impl1.deinit(context);

	return E_MYOS_SUCCESS;
}

myos_result_t DisplayComponent::queryInterface(
		MyOS::Core::IComponent& requestor, const uuid_t& uuid,
		MyOS::Core::IInterface*& result) {

	return E_MYOS_E_NOINTERFACE;
}

}
}
} // namespaces
