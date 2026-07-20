#include "IContext.h"
#include "MyOSCoreContainer.h"

#include "module.h" // direct call to initModule, test

namespace MyOS {
namespace Init {

MyOSCoreContainer* MyOSCoreContainer::instance INITINSTANCE;

MyOSCoreContainer::MyOSCoreContainer() :
	IContainer(VERSION(2, 1), (IContainer*&) instance )

	, mM(*this)
	, iH(*this)
	//, demandPaging(*this)
	, display(*this)
	, timer(*this)
	, mT(*this)
	, mP(*this)
	, ld(*this)
    , zip(*this)

#ifdef TEST_IN_CORE
	, test(*this)
#endif
{

}

// @todo use exceptions here instead of ifs
myos_result_t
MyOSCoreContainer::initialize(Context::IContext& context, NVPAIR params[]) {
	myos_result_t r;

	// Once display is initialized, debug output can be used
	if ((r=display.initialize(context, params))!=E_MYOS_SUCCESS)
	    return r;

    CHECKPOINT( '1' );
    if ((r=mM.initialize(context, params))!=E_MYOS_SUCCESS)
        return r;
    CHECKPOINT( '2' );
	if ((r=iH.initialize(context, params))!=E_MYOS_SUCCESS)
	    return r;
    CHECKPOINT( '3' );
	if ((r=timer.initialize(context, params))!=E_MYOS_SUCCESS)
		return r;
    CHECKPOINT( '4' );
	if ((r=mT.initialize(context, params))!=E_MYOS_SUCCESS)
		return r;
    CHECKPOINT( '5' );
	if ((r=mP.initialize(context, params))!=E_MYOS_SUCCESS)
		return r;
    CHECKPOINT( '6' );
    if ((r=ld.initialize(context, params))!=E_MYOS_SUCCESS)
        return r;
    CHECKPOINT( '7' );
//    if ((r=zip.initialize(context, params))!=E_MYOS_SUCCESS)
//        return r;
//    CHECKPOINT( '8' );


#ifdef TEST_IN_CORE
	if ((r=test.initialize(context, params))!=E_MYOS_SUCCESS)
		return r;
#else

	// Now initialize Atmosphere as a separate module
	ld.loadInitialModule( context );
	// initModule( context );
#endif

	// Could save reference to process
	return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t MyOSCoreContainer::deinitialize(Context::IContext& context)
{
	// test.deinitialize(context);
    // zip.deinitialize(context);
    ld.deinitialize(context);
	mP.deinitialize(context);
	mT.deinitialize(context);
	timer.deinitialize(context);
	display.deinitialize(context);
	//demandPaging.deinitialize(context);
	iH.deinitialize(context);
	//repository.deinitialize(context);
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
