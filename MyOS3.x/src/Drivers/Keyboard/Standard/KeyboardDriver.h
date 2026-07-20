#ifndef KeyboardDriver_H
#define KeyboardDriver_H

#include "Drivers/CDriverBase.h"
#include "Devices/Keyboard/IKeyboard.h"


namespace MyOS { namespace Drivers { namespace Keyboard { namespace Standard {

/****
 * Dependencies
 ***/
using MyOS::Core::IComponent;
using MyOS::Drivers::IDriverManager;
using InterruptHandling::IInterruptHandling;
using Devices::Keyboard::IKeyboard;

class KeyboardDriver : public MyOS::Drivers::CDriverBase {

// public is easier than declaring friends...
public:
	inline static const char* const ID() { return "3cc231d8-2c39-4bf5-80ce-5427e7b5b549"; }

IInterruptHandling* iInterruptHandling;
KeyboardDriver( MyOS::Core::IContainer& container ) throw() INITSECTION;
    virtual myos_result_t initialize( Context::IContext& context, NVPAIR params[] ) INITSECTION;
    virtual myos_result_t deinitialize( Context::IContext& context );

	// CDriverBase
    virtual myos_result_t detectDevices() INITSECTION;



private:
    static KeyboardDriver* instance;
    
public: inline static KeyboardDriver& getInstance() { return *instance; }

private:    
    IKeyboard* impl1[1];

};

}}
}} // namespaces



#endif
