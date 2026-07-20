#include "Context/IContext.h"
#include "IHComponent.h"

namespace MyOS {
namespace InterruptHandling {

//const uuid_t IHComponent::ID = "ea21dd8e-ce8a-4777-8f03-5f0e39283c2c";
IHComponent* IHComponent::instance INITINSTANCE;

IHComponent::IHComponent(MyOS::Core::IContainer& container) throw() :
	IComponent(container, VERSION(1, 1), ID(), (IComponent*&) instance )

	, impl1(*this)

{
}

myos_result_t IHComponent::initialize(Context::IContext& context, NVPAIR params[]) {

	if (!impl1.init(context, params))
		return E_MYOS_E_FAIL;

	if (context.add(impl1)!=E_MYOS_SUCCESS)
		return E_MYOS_E_REGFAILED;

	return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t IHComponent::deinitialize(Context::IContext& context) {

	context.remove(impl1 );

	impl1.deinit(context);

	return E_MYOS_SUCCESS;
}

myos_result_t IHComponent::queryInterface(MyOS::Core::IComponent& requestor,
		const uuid_t& uuid, MyOS::Core::IInterface*& result) {

	return E_MYOS_E_NOINTERFACE;
}

}
} // namespaces
