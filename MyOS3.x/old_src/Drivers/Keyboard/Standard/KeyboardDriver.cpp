#include "Context/IContext.h"
#include "KeyboardDriver.h"
namespace MyOS
{
namespace Drivers
{
namespace Keyboard
{
namespace Standard
{
KeyboardDriver* KeyboardDriver::instance INITINSTANCE;

KeyboardDriver::KeyboardDriver(MyOS::Core::IContainer& container) throw() :
    MyOS::Drivers::CDriverBase(container, VERSION(2, 1), ID(),
            (IComponent*&) instance )
{
}

myos_result_t KeyboardDriver::initialize(Context::IContext& context, NVPAIR params[])
{
    iInterruptHandling = (IInterruptHandling*) &context.lookup( 
            myos_name_t( IInterruptHandling::ID() ) );
    return CDriverBase::init(context);
}

/** Also called after partial init() that fails */
myos_result_t KeyboardDriver::deinitialize(Context::IContext& context)
{

    CDriverBase::remove();

    // if (iInterruptHandling) iInterruptHandling->release();

    return E_MYOS_SUCCESS;
}

}
}
}
} // namespaces
