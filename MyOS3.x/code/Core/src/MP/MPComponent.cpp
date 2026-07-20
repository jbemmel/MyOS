#include "IContext.h"
#include "MPComponent.h"

namespace MyOS {
namespace MultiProcessing {

//const uuid_t MPComponent::ID = "ba2161d8-487c-426d-8465-64e0f722ce40";

MPComponent* MPComponent::instance INITINSTANCE;

MPComponent::MPComponent(MyOS::Core::IContainer& container) throw() :
	IComponent(container, VERSION(1, 1), ID(), (IComponent*&) instance )
	, impl1(*this)
{
}

myos_result_t MPComponent::initialize(IContext& context, NVPAIR params[]) {
    iVirtualMemory  = (IVirtualMemory*) &context.lookup( myos_name_t( IVirtualMemory::ID() ) );
	iMultiThreading = (IMultiThreading*) &context.lookup(myos_name_t(IMultiThreading::ID() ) );

	if (!impl1.init(context, params))
		return E_MYOS_E_FAIL;

	if (context.add(impl1)!=E_MYOS_SUCCESS)
		return E_MYOS_E_REGFAILED;

	return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t MPComponent::deinitialize(Context::IContext& context) {

	context.remove(impl1 );

	impl1.deinit(context);

//	if (iMultiThreading)
//		iMultiThreading->release();
//
//	if (iPhysicalMemory)
//		iPhysicalMemory->release();
//
//	if (iVirtualMemory)
//		iVirtualMemory->release();

	return E_MYOS_SUCCESS;
}

myos_result_t MPComponent::queryInterface(MyOS::Core::IComponent& requestor,
		const uuid_t& uuid, MyOS::Core::IInterface*& result) {

	return E_MYOS_E_NOINTERFACE;
}

}
} // namespaces
