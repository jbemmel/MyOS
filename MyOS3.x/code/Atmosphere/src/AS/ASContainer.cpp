#include "IContext.h"
#include "ASContainer.h"
#include "IMultiProcessing.h"

// Atmosphere now built as module
#include "module.h"

DECLARE_DISPLAY

// This optimizes dynamic linking ( 149x referenced )
extern "C" void __cxa_pure_virtual() { 
    BUG("__cxa_pure_virtual called"); 
}

/**
 * Main start routing for AS Container
 */
int initModule( MyOS::Context::IContext &context )
{
    INIT_DISPLAY( context );
    
    /// Declare static instance here
    static MyOS::Atmosphere::ASContainer as;
    
    /**
     * Start Atmosphere as a separate process
     *
    using MultiProcessing::IMultiProcessing;
    IMultiProcessing& mp = context.lookup( 
            myos_name_t(IMultiProcessing::ID())).castToExcept<IMultiProcessing>();
    NVPAIR bootparams[2] = { NVPAIR("boot", &context), NVPAIR() };
    mp.createProcess( as, bootparams, 1 );
    */
    
    as.initialize( context, 0 );
}

namespace MyOS {
namespace Atmosphere {

using Context::IContext;

ASContainer* ASContainer::instance INITINSTANCE;

ASContainer::ASContainer() :
	IContainer(VERSION(2, 1), (IContainer*&) instance )	
	, rc(*this)

	, driverManager(*this)
    , ne2000(*this)
    , ip(*this)
    , tcp(*this)
    , dhcp(*this)
    
    , test(*this)
{
    
}

// virtual 
int 
ASContainer::run( NVPAIR params[] )
{
    DPRINTK( "\n#3f#Atmosphere initializing..." );
    
    // BootContext passed as a parameter
    IContext* boot = (IContext*) params[0].getValue();
    
    return this->initialize( *boot, params );
}

// Called from ::run
myos_result_t 
ASContainer::initialize(Context::IContext& context, NVPAIR params[]) {

    myos_result_t r;

	if ((r=rc.initialize(context, params))!=E_MYOS_SUCCESS)
	    return r;

	if ((r=driverManager.initialize(rc.getContext(), params))
	            !=E_MYOS_SUCCESS)
        return r;

    if ((r=ne2000.initialize(rc.getContext(), params))!=E_MYOS_SUCCESS)
        return r;
    
    if ((r=ip.initialize(rc.getContext(), params))!=E_MYOS_SUCCESS)
        return r;

    if ((r=tcp.initialize(rc.getContext(), params))!=E_MYOS_SUCCESS)
        return r;

    if ((r=dhcp.initialize(rc.getContext(), params))!=E_MYOS_SUCCESS)
        return r;
    
    if ((r=test.initialize(rc.getContext(), params))!=E_MYOS_SUCCESS)
        return r;
    
	return E_MYOS_SUCCESS;
}

/** Also called after partial init() that fails */
myos_result_t ASContainer::deinitialize(Context::IContext& context) 
{
	rc.deinitialize(context);

	return E_MYOS_SUCCESS;
}

myos_result_t ASContainer::queryComponent(
		MyOS::Core::IComponent& requestor, const uuid_t& uuid,
		MyOS::Core::IComponent*& result) {

	return E_MYOS_E_NOINTERFACE;
}

myos_result_t ASContainer::queryInterface(
		MyOS::Core::IComponent& requestor, const uuid_t& uuid,
		MyOS::Core::IInterface*& result) {

	return E_MYOS_E_NOINTERFACE;
}

myos_result_t ASContainer::destroyComponent(IComponent& c) {
	return E_MYOS_E_NOTIMPL;
}

}} // namespaces


