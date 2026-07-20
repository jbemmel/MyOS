#include "Context/IContext.h"
#include "FloppyDriver.h"

#include "TimerFacility/ITiming.h"

namespace MyOS
{
namespace Drivers
{
namespace Floppy
{
namespace Default
{
FloppyDriver* FloppyDriver::instance INITINSTANCE;

FloppyDriver::FloppyDriver(MyOS::Core::IContainer& container) throw() :
    MyOS::Drivers::CDriverBase(container, VERSION(2, 1), ID(),
            (IComponent*&) instance )
{
}

myos_result_t FloppyDriver::initialize(Context::IContext& context, NVPAIR params[])
{
    iVirtualMemory  = (IVirtualMemory*) &context.lookup( myos_name_t( IVirtualMemory::ID() ) );

    iInterruptHandling = (IInterruptHandling*) &context.lookup( myos_name_t( IInterruptHandling::ID() ) );

    iTiming = (ITimerFacility*) &context.lookup(myos_name_t(ITimerFacility::ID() ) );

    allocator.init( *iVirtualMemory, 0);

    return CDriverBase::init(context);
}

/** Also called after partial init() that fails */
myos_result_t FloppyDriver::deinitialize(Context::IContext& context)
{

    CDriverBase::remove();

    // if (iTiming) iTiming->release();

    // if (iInterruptHandling) iInterruptHandling->release();

    // if (iVirtualMemory) iVirtualMemory->release();

    return E_MYOS_SUCCESS;
}

}
}
}
} // namespaces
