#include "module.h"

#include "IContext.h"
#include "debug.h"

#include "IVirtualMemory.h"
#include "MM/ByteAllocator.h"

DECLARE_DISPLAY;

struct SomeException {
    int value;

    inline SomeException( int i ) throw() : value(i) {}

    virtual void testMethodTableRelocation() {
        PRINTK( "\nVirtual method relocation OK" );
    }
};

static int test_bss[100];

static int exceptionThrower() throw (SomeException) NOINLINE;
static int exceptionCatcher() throw () NOINLINE;

using MyOS::MM::IVirtualMemory;

// MYOS_MODULE_PUBLIC __declspec(dllexport)
int initModule( MyOS::Context::IContext& context )
// INIT_MODULE( MyOS::Context::IContext& context )
{

    INIT_DISPLAY( context );

// #define PRINTK MyOS::Devices::Display::DisplayComponent::getInstance().cout().printf

    PRINTK( "\n#2f#Hello world from a separate module! test_bss=%d", test_bss[99] );
    ASSERTION( test_bss[99]==0, E_ERROR );

/*
    IVirtualMemory *iVirtualMemory = context.lookup(
            MyOS::myos_name_t(IVirtualMemory::ID()) ).castTo<IVirtualMemory>();

    MyOS::MM::ByteAllocator allocator;
    allocator.init( *iVirtualMemory, 0);
    void *m = allocator.allocate( 100 );
    allocator.deallocate( m, 100 );
*/
    return exceptionCatcher();
}

int exceptionCatcher() throw()
{
    try {
        exceptionThrower();
    } catch (SomeException &e) {
        PRINTK( "\n#2f#Exception caught, value=%d", e.value );
        e.testMethodTableRelocation();
    } catch (...) {
        PRINTK( "\n#2f#Some exception caught" );
    }

    return 1;
}



int exceptionThrower() throw (SomeException) {
    PRINTK( "\nexceptionThrower" );
    throw SomeException(1234);
}
