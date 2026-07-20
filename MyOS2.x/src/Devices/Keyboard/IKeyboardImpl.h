#ifndef IKeyboardImpl_H
#define IKeyboardImpl_H

#include "Devices/Keyboard/IKeyboard.h"

// for KeyboardComponent
#include "InterruptHandling/IInterruptHandling.h"

#include "Devices/IOPort.h"

namespace MyOS { namespace Devices { namespace Keyboard {

   using Context::IContext;
   using namespace InterruptHandling;

class IKeyboardImpl : public IKeyboard, public IInterruptContext 
{
public:
   IKeyboardImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( ) INITSECTION;
   void deinit( );

   // INC
   virtual myos_result_t installHandler( IKeyboardHandler& h );
   virtual myos_result_t removeHandler( IKeyboardHandler& h );

   // XML
   virtual  myos_result_t sendKey( u32 key );

   // IInterruptContext
   virtual void onInterrupt( );

private:
   enum E_LEDS {
      E_LED_CAPS     = (1<<2),
      E_LED_NUM      = (1<<1),
      E_LED_SCROLL   = (1<<0),
   };

   void setLeds( u8 leds ) {
      while (status & E_CTRLDATA_PRESENT);   // wait until ready
      toKeyboard = E_SET_LEDS;
      while (status & E_CTRLDATA_PRESENT);   // wait until ready
      toKeyboard = leds;
   }

   /**
    * Selects the scancode set used by the keyboard
    * @param  setid: 0,1,2,3 where 0 returns current, 1-3 select set
    * 
    * Note: Do this *before* IRQs are enabled!
    */
   u8 setScanCodeSet( u8 setid ) {
      while (status & E_CTRLDATA_PRESENT);   // wait until ready
      toKeyboard = E_SELECT_SCANSET;
      while (status & E_CTRLDATA_PRESENT);   // keyboard will send ACK
      toKeyboard = setid;
      while (status & E_CTRLDATA_PRESENT);   // keyboard will send ACK
      while (keydata==E_KEYBOARD_ACK);       // eat ACKs
      return keydata;    // 'C'=1, 'A'=2, '?'=3
   }

private:
   IOPort8_W<0x60> toKeyboard;
   IOPort8_R<0x60> keydata;
   IOPort8_W<0x64> control;
   IOPort8_R<0x64> status;

   enum E_MISC {
      E_KEYBOARD_ACK = 0xfa
   };

   enum E_STATUSBITS {
      E_PARITY_ERR      = (1<<7),
      E_TIMEOUT_ERR     = (1<<6),

      E_AUX_DATA_KEYB   = (1<<5),   // aux buffer contains keyboard data
      E_KEYBOARD_FREE   = (1<<4),
      E_CTRL_WRITTEN    = (1<<3),   // byte written on port 0x64
      E_SYSCHECK_OK     = (1<<2),

      E_CTRLDATA_PRESENT = (1<<1),
      E_KEYBDATA_PRESENT = (1<<0),
   };

   enum E_COMMAND {
      E_WRITE_COMMAND   = 0x60,     // from bochs

      E_DEACTIVATE_2ND  = 0xa7,
      E_ACTIVATE_2ND    = 0xa8,

      E_SELFTEST           = 0xaa,  // 0x55 in keydata if success
      E_DISABLE_KEYBOARD   = 0xad,
      E_ENABLE_KEYBOARD    = 0xae,

      E_READ_INPUT      = 0xc0,
      E_READ_OUTPUT     = 0xd0,  // data in keydata
      E_WRITE_OUTPUT    = 0xd1,

      E_SET_LEDS        = 0xed,
      E_SELECT_SCANSET  = 0xf0,  // 
   };

   void sendCommand( E_COMMAND command ) {
      while (status & E_CTRLDATA_PRESENT);
      control = command;
   }

   u32 stickyStatus;  // status of keys like NUMLOCK, CAPSLOCK, etc
                      // Also temp status of SHIFT, CTRL, ALT (break clears)
   IKeyboardHandler* handler;    // guaranteed != 0

   // for easy implementation of getch
   // MultiThreading::t_syncpoint charReady;

   class DummyHandler : public IKeyboardHandler
   {

      // IKeyboardHandler, ignored
      virtual void onKey( u32, u32 ) { }
   } dummy;   
};
   
}
}} // namespaces
#endif
