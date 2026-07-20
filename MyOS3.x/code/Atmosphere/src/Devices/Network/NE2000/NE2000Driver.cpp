#include "IContext.h"
#include "NE2000Driver.h"
#include "DP8390d.h"

namespace MyOS {
namespace Drivers {
namespace Network {
namespace NE2000 {

NE2000Driver* NE2000Driver::instance INITINSTANCE;

NE2000Driver::NE2000Driver(MyOS::Core::IContainer& container) throw() :
    MyOS::Drivers::CDriverBase(container, VERSION(2, 1), ID(),
            (IComponent*&) instance )
{
}

myos_result_t NE2000Driver::initialize(Context::IContext& context,
        NVPAIR params[])
{
    iVirtualMemory  = (IVirtualMemory*) &context.lookup( myos_name_t( IVirtualMemory::ID() ) );

    iInterruptHandling = (IInterruptHandling*) &context.lookup( myos_name_t( IInterruptHandling::ID() ) );

    iMultiThreading = (IMultiThreading*) &context.lookup(myos_name_t(IMultiThreading::ID() ) );

    allocator.init( *iVirtualMemory, 0);

    return CDriverBase::init(context);
}

/** Also called after partial init() that fails */
myos_result_t NE2000Driver::deinitialize(Context::IContext& context)
{

    CDriverBase::remove();

    // if (iVirtualMemory) iVirtualMemory->release();

    if (impl1[0]) {
        delete impl1[0];
        impl1[0] = 0;
    }
    
    return E_MYOS_SUCCESS;
}

// virtual 
myos_result_t 
NE2000Driver::detectDevices() // INITSECTION;
{
    using Devices::Network::NE2000::DP8390D;
    
    DP8390D* chip = new DP8390D( *this );
    if (chip->init()) {
        iDriverManager->registerDevice( Drivers::E_DEV_ETH, *chip );
        impl1[0] = chip;
        return E_MYOS_SUCCESS;
    }
    return E_MYOS_ERROR;    
}

}}}} // namespaces
