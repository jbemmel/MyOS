#include "IContext.h"
#include "TimerComponent.h"

namespace MyOS {
namespace Timer {

//const uuid_t TimerComponent::ID = "a000f904-ce87-4c4a-a055-804df647ec5b";
TimerComponent* TimerComponent::instance INITINSTANCE;

TimerComponent::TimerComponent(MyOS::Core::IContainer& container) throw() :
	IComponent(container, VERSION(2, 1), ID(), (IComponent*&) instance )

	, impl1(*this)

{
}

myos_result_t TimerComponent::initialize(IContext& context, NVPAIR params[]) {
    iVirtualMemory  = (IVirtualMemory*) &context.lookup( myos_name_t( IVirtualMemory::ID() ) );
    iInterruptHandling = (IInterruptHandling*) &context.lookup(myos_name_t(IInterruptHandling::ID()) );

	allocator.init( *iVirtualMemory, 0);

	if (!impl1.init(context, params))
		return E_MYOS_E_FAIL;

	if (context.add(impl1)!=E_MYOS_SUCCESS)
		return E_MYOS_E_REGFAILED;

	return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t TimerComponent::deinitialize(Context::IContext& context) {

	context.remove(impl1 );

	impl1.deinit(context);

//	if (iInterruptHandling)
//		iInterruptHandling->release();
//
//	if (iVirtualMemory)
//		iVirtualMemory->release();

	return E_MYOS_SUCCESS;
}

myos_result_t TimerComponent::queryInterface(MyOS::Core::IComponent& requestor,
		const uuid_t& uuid, MyOS::Core::IInterface*& result) {

	return E_MYOS_E_NOINTERFACE;
}

}
} // namespaces
