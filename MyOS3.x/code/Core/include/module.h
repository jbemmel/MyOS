/**
 * Support for dynamically loaded kernel modules and applications
 */

#ifndef MODULE_H
#define MODULE_H

#include "IDisplay.h"

/**
 * initModule - Modules must define at least this function
 *
 * It is declared "extern C" to avoid C++ name mangling.
 */
extern "C" int initModule( MyOS::Context::IContext& context ) REGPARM(1);

// #define INIT_MODULE( ps ) extern int initModule( ps ) ASMNAME("_initModule"); \
//    int initModule( ps )

namespace MyOS {
namespace Module {

/**
 * To avoid excessive dynamic linking, each module gets its own instance of a
 * pointer to DisplayComponent::instance
 */
extern Devices::Display::IDisplay *display;

}
}

#define DECLARE_DISPLAY \
    namespace MyOS { namespace Module { MyOS::Devices::Display::IDisplay *display = 0; }}

// Use this in initModule
#define INIT_DISPLAY(c) \
    MyOS::Module::display = c.lookup( MyOS::myos_name_t( MyOS::Devices::Display::IDisplay::ID() ) ) \
      .castTo<MyOS::Devices::Display::IDisplay>();

#endif
