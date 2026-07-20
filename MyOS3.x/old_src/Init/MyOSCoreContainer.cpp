#include "Context/IContext.h"
#include "MyOSCoreContainer.h"

namespace MyOS {
namespace Init {

MyOSCoreContainer* MyOSCoreContainer::instance INITINSTANCE;

MyOSCoreContainer::MyOSCoreContainer() :
	IContainer(VERSION(2, 1), (IContainer*&) instance )
	
	, mM(*this)
	, repository(*this)
	, iH(*this)
	, demandPaging(*this)
	, display(*this)
	, timer(*this)
	, mT(*this)
	, mP(*this)
	, driverManager(*this)
	, nE2000(*this)
	, kb( *this )
	, iP(*this)
	, tCP(*this)
	, bios(*this)
	, test(*this)
{
    
}

myos_result_t MyOSCoreContainer::initialize(Context::IContext& context, NVPAIR params[]) {
	myos_result_t r;

	if ((r=mM.initialize(context, params))!=E_MYOS_SUCCESS)
		return r;

	if ((r=repository.initialize(context, params))!=E_MYOS_SUCCESS)
		return r;

	// Once display is initialized, debug output can be used
    if ((r=display.initialize(repository.getContext(), params))!=E_MYOS_SUCCESS)
        return r;
	CHECKPOINT( '1' );
		
	if ((r=iH.initialize(repository.getContext(), params))!=E_MYOS_SUCCESS)
		return r;
	CHECKPOINT( '2' );
			
	// demandPaging is really an _optional_ core component, it can be removed or done later
	if ((r=demandPaging.initialize(repository.getContext(), params))!=E_MYOS_SUCCESS)
		return r;
	CHECKPOINT( '3' );

	if ((r=timer.initialize(repository.getContext(), params))!=E_MYOS_SUCCESS)
		return r;

	if ((r=mT.initialize(repository.getContext(), params))!=E_MYOS_SUCCESS)
		return r;

	if ((r=mP.initialize(repository.getContext(), params))!=E_MYOS_SUCCESS)
		return r;

	if ((r=driverManager.initialize(repository.getContext(), params))
			!=E_MYOS_SUCCESS)
		return r;

	if ((r=nE2000.initialize(repository.getContext(), params))!=E_MYOS_SUCCESS)
		return r;
    if ((r=kb.initialize(repository.getContext(), params))!=E_MYOS_SUCCESS)
        return r;
	
	if ((r=iP.initialize(repository.getContext(), params))!=E_MYOS_SUCCESS)
		return r;

	if ((r=tCP.initialize(repository.getContext(), params))!=E_MYOS_SUCCESS)
		return r;

	// @note Initializing BIOS breaks null pointer detection!	
	if ((r=bios.initialize(repository.getContext(), params))!=E_MYOS_SUCCESS)
	    return r;
	
	if ((r=test.initialize(repository.getContext(), params))!=E_MYOS_SUCCESS)
		return r;

	return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t MyOSCoreContainer::deinitialize(Context::IContext& context) 
{
	test.deinitialize(context);
	bios.deinitialize(context);
	tCP.deinitialize(context);
	iP.deinitialize(context);
	kb.deinitialize(context);
	nE2000.deinitialize(context);
	driverManager.deinitialize(context);
	mP.deinitialize(context);
	mT.deinitialize(context);
	timer.deinitialize(context);
	display.deinitialize(context);
	demandPaging.deinitialize(context);
	iH.deinitialize(context);
	repository.deinitialize(context);
	mM.deinitialize(context);

	return E_MYOS_SUCCESS;
}

myos_result_t MyOSCoreContainer::queryComponent(
		MyOS::Core::IComponent& requestor, const uuid_t& uuid,
		MyOS::Core::IComponent*& result) {

	return E_MYOS_E_NOINTERFACE;
}

myos_result_t MyOSCoreContainer::queryInterface(
		MyOS::Core::IComponent& requestor, const uuid_t& uuid,
		MyOS::Core::IInterface*& result) {

	return E_MYOS_E_NOINTERFACE;
}

myos_result_t MyOSCoreContainer::destroyComponent(IComponent& c) {
	return E_MYOS_E_NOTIMPL;
}

}
} // namespaces
