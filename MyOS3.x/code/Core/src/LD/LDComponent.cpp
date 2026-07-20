#include "IContext.h"
#include "LDComponent.h"

namespace MyOS {
namespace DynamicLoading {

LDComponent* LDComponent::instance INITINSTANCE;

LDComponent::LDComponent(MyOS::Core::IContainer& container) throw() :
	IComponent(container, VERSION(1, 1), ID(), (IComponent*&) instance )

	, impl1(*this)

{
}

myos_result_t LDComponent::initialize(IContext& context, NVPAIR params[]) {
    iVirtualMemory  = (IVirtualMemory*) &context.lookup( myos_name_t( IVirtualMemory::ID() ) );
    // allocator.init( *iVirtualMemory, 0);

	if (context.add(impl1)!=E_MYOS_SUCCESS)
		return E_MYOS_E_REGFAILED;

	return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t LDComponent::deinitialize(Context::IContext& context)
{
	context.remove(impl1 );
	return E_MYOS_SUCCESS;
}

myos_result_t LDComponent::queryInterface(MyOS::Core::IComponent& requestor,
		const uuid_t& uuid, MyOS::Core::IInterface*& result) {

	return E_MYOS_E_NOINTERFACE;
}

}
} // namespaces
