#include "IKeyboardImpl.h"
#include "Drivers/Keyboard/Standard/KeyboardDriver.h"
#include "KeyboardCodes.h"
#include "MM/new.h"

#include "debug.h"

namespace MyOS { namespace Devices { namespace Keyboard {

using Drivers::Keyboard::Standard::KeyboardDriver;

IKeyboardImpl::IKeyboardImpl( MyOS::Core::IComponent& c ) // INITSECTION
: IKeyboard( c, VERSION(1,0) ), handler(&dummy)
{
   // :)
   // setLeds( E_LED_CAPS | E_LED_NUM | E_LED_SCROLL );   // test
}

bool 
IKeyboardImpl::init(  ) /* INITSECTION */
{
    // !before! IRQs are enabled
    u8 s = setScanCodeSet(0);
    DPRINTK( "\nKBD: Current scancode set is %X", s );
  
    sendCommand( E_WRITE_COMMAND );
    toKeyboard = 0x01 | 0x40;   // allow IRQ1, enable scan convert
    stickyStatus = 0;
    KeyboardDriver::getInstance().iInterruptHandling->setIRQHandler( E_IRQ1, *this, E_NONE );

    return true;
}

void 
IKeyboardImpl::deinit( )
{
   sendCommand( E_DISABLE_KEYBOARD ); 
   KeyboardDriver::getInstance().iInterruptHandling->removeIRQHandler( E_IRQ1 );
}

// virtual
myos_result_t
IKeyboardImpl::installHandler( IKeyboardHandler& h )
{
   if (handler==&dummy) {
      handler = &h;
      return E_MYOS_SUCCESS;
   }
   return E_MYOS_E_FAIL;
}

// virtual
myos_result_t
IKeyboardImpl::removeHandler( IKeyboardHandler& h )
{
   if (handler==&h) {
      handler = &dummy;
      return E_MYOS_SUCCESS;
   }
   return E_MYOS_E_FAIL;
}

/**
 * Scancode handling (see http://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html)
 * -----------------
 * The reasons why this is so complex are historical, but most are overseeable
 * Two special keys require special attention:
 * PrintScreen -> { e0 2a e0 37 }, or { e0 37 } with <CTRL> or <SHIFT>,
 *                                 or { 54 } with <ALT>
 *   Without special care, { 2a 37 } = <LSHIFT> * and { 0x37 } = *
 *
 * Pause -> { e1 1d 45 e1 9d c5 }, or { e0 46 e0 c6 } when pressed with <CTRL>
 *   and no break codes
 *   Without special care, { 1d 45 } = <CTRL> <NUMLOCK> 
 *   and { 0xE0 0x46 } = <SCROLLLOCK>, while it should be CTRL-BREAK
 * 
 *   Note that Bochs does not mimmick this behavior very well, maybe because
 *   Windows traps such special keystrokes before Bochs can
 *
 * Current implementation: K_PRINTSCREEN is only reported with <ALT>, else "*"
 * Pause: { 0xe1 ... } by if, { 0xe0 ... } by detecting 0xc6 & undoing SCROLLOCK
 * 
 * This assumes MF-2 scancodes, which should be the default set for the keyboard
 * after boot
 */

// virtual
void
IKeyboardImpl::onInterrupt(  )
{
   u32 escape=0;
   // DPRINTK( "\nStart" );

   // PRINTF( "\nKeyboard interrupt status=%X", status );
   while ( status&E_KEYBDATA_PRESENT) {
      u8 d = keydata;
      // DPRINTK( " code=%X", d );
      KeyCode kc(0);
      if ((d & 0x80)==0) { // only make codes
         kc = KeyCode::interpret(d,stickyStatus | escape);
         // DPRINTK( "\n%u->%c (%x)", d, (char) kc, (u32) kc );

         // check for status changing keys
         if (u32 special = kc.getSpecialBits()) {

            if (special&R_MODIFYKEYS) {
               stickyStatus |= special;
               continue;   // or return 0 ?  
            } else {
               
               // special case: CTRL-BREAK gives 0x1D 0xE0 0x46
               // while 0x46 normally means scrolllock...
               if (d==0x46 && (stickyStatus&A_CTRL)) {
                  DPRINTK( "#BREAK#" );
                  kc = A_CTRL | K_BREAK;  // both ctrl, break key                 
               } else {               
                  stickyStatus ^= special;
                  // DPRINTK( " <STICKY mask=%x> bit=%x new state=%x\n", R_STICKYKEYS, special, stickyStatus );                        
                  return;   // abort, ok?
               }                  
            }            
         } else {
            // dont print shift pressed ed
            // DPRINTK( "%c", (char) kc );
         }
         escape = 0;
      } else if (d==0xe0) {   // escape code for grey keys
         escape = E_ESCAPE;
         // DPRINTK( " <GREY_ESC>" );
         continue;
      } else switch (d) {
         case 0xaa:  // break code for LSHIFT
            stickyStatus &= ~K_LSHIFT; continue;   // or return 0 ?
         case 0xb6:  // break code for RSHIFT
            stickyStatus &= ~K_RSHIFT; continue;   // or return 0 ?

         case 0xb8:  // break code for LALT, RALT with \ESC (could distinct)
            // Could implement ALT codes, and send it here
            stickyStatus &= ~A_ALT; continue;      // or return 0 ?
         
         case 0xe1:  // escape code for PAUSE key, special!
            for (u32 c=5; c>0; --c) {  // absorb 5 codes
               while (!(status & E_KEYBDATA_PRESENT));
               d = keydata;
            }
            DPRINTK( "\n<PAUSE>" );
            kc = K_PAUSE;
            stickyStatus ^= S_PAUSE;
            break;   // notify handler of 'Pause' key press

         case 0x9d:  // break code for CTRL (left, right comes with \ESC)
            stickyStatus &= ~A_CTRL;   // reset control sticky bits
            continue;
      }

      // notify client (cooked mode), also toggles of sticky bits are sent
      if (kc.getKeyBits()) {
         // Note: This aborts reading of more keys
         /* return */ handler->onKey( kc.getKeyBits(), stickyStatus );
      }
   }

   // DPRINTK( "\nExit IRQ handler" );
}


// virtual 
myos_result_t
IKeyboardImpl::sendKey( u32 key )  {
   handler->onKey( key, stickyStatus );   // what if returns thread?
   return E_MYOS_SUCCESS;
}

}}}  // namespaces

