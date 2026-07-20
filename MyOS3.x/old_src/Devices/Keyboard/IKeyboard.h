#ifndef IKeyboard_H
#define IKeyboard_H

#include "Devices/ICharDevice.h"

namespace MyOS { namespace Devices { namespace Keyboard {

/// Bits that can be received by the keyboard handler, 32bit values
enum E_KeyboardCode {

    // sign bit: make/break
    K_MAKECODE  = 0x00000000,
    K_BREAKCODE = 0x80000000,

    // bits 24..30: control keys, (L >> 1) == R
    K_LSHIFT = (1<<30),
    K_RSHIFT = (1<<29),
    K_LALT   = (1<<28),
    K_RALT   = (1<<27),
    K_LCTRL  = (1<<26),
    K_RCTRL  = (1<<25),

    E_ESCAPE = (1<<24),  // set if escape code was seen, used internally

    A_CTRL  = K_RCTRL   | K_LCTRL,
    A_ALT   = K_RALT    | K_LALT,
    A_SHIFT = K_RSHIFT  | K_LSHIFT,

    // bits 16..23: sticky keys
    S_CAPSLOCK   = (1<<22),   // led 2
    S_NUMLOCK    = (1<<21),   // led 1
    S_SCROLLLOCK = (1<<20),   // led 0
    S_PAUSE      = (1<<19),
    S_INSERT     = (1<<18),

   // 2 bit room here
   
   /**
    * Note: I also put SHIFT, CTRL and ALT here, although they are not really
    * longterm sticky bits
    * CTRL is needed for detecting CTRL-BREAK (break code for ctrl is sent twice)
    */
   R_STICKYKEYS     = S_NUMLOCK|S_CAPSLOCK|S_SCROLLLOCK|S_PAUSE|S_INSERT,
   R_MODIFYKEYS     = A_SHIFT|A_CTRL|A_ALT,
   R_SPECIALKEYS    = R_STICKYKEYS | R_MODIFYKEYS,

   // to select the key value without control key information
   R_KEYMASK   = 0x0000FFFF,

   // bits 8..15: control keys like F1..F12, cursor movement
   K_F1=0x100, K_F2=0x200, K_F3=0x300, K_F4=0x400,
   K_F5=0x500, K_F6=0x600, K_F7=0x700, K_F8=0x800,
   K_F9=0x900, K_F10=0xA00, K_F11=0xB00, K_F12=0xC00,

   // cursor keys, in order of scancodes (not needed)
   K_HOME=0x0D00,K_UP=0x0E00,K_PAGEUP=0x0F00,     // 7,8,9
   K_LEFT=0x1000,K_CENTER=0x1100,K_RIGHT=0x1200,  // 4,5,6
   K_END=0x1300,K_DOWN=0x1400,K_PAGEDOWN=0x1500,  // 1,2,3

   // begin/end of cursor movement values (count TAB/BS too ?)
   R_CURSORMOVEMENT = K_HOME,
   R_CURSORMOVEND   = K_PAGEDOWN,

   // some misc cursor movement
   K_DEL       = 0x1600,

   // other special keys
   K_ESCAPE = 0x1700,
   K_SYSREQ = 0x1800,   // combo with <ALT> and <PRINTSCREEN>
   K_BREAK  = 0x1900,   // combo with <CTRL>
   K_ALTGR  = 0x1A00,

   // sticky status as keys
   K_CAPSLOCK   = 0x1B00,
   K_NUMLOCK    = 0x1C00,
   K_SCROLLLOCK = 0x1D00,
   K_PAUSE      = 0x1E00,
   K_INSERT     = 0x1F00,

   // bits 0..7: normal (printable) ASCII characters castable to char

   K_ENTER     = '\n',  // must be printable
   K_BACKSPACE = '\b',
   K_TAB       = '\t',
};

/// Received "cooked" keyboard input
/**
 * I choose to let the receiver of keys determine if switching to a
 * separate thread context is needed or not. The keyboard handler cannot
 * determine this, if only some simple action is done (eg echo char on 
 * screen and remember it in some buffer) then there is no need for 
 * synchronization overhead
 */
class IKeyboardHandler
{
public:
   /**
    * Called when an input-producing key is pressed
    * @param keybits : OR'ed bits of E_KeyboardCode
    */
   virtual void onKey( u32 key, u32 stickyStatus ) = 0;
};

SUB_INTERFACE( IKeyboard, ICharDevice, "0f8c2323-3fea-4a9e-b540-4b5ec7592697" )

    virtual  myos_result_t sendKey( u32 key )  = 0;

    /**
     * Installs a callback for receiving cooked keyboard input
     */
    virtual myos_result_t installHandler( IKeyboardHandler& h ) = 0;
    
    /**
     * Deinstalls the previously set callback
     */
    virtual myos_result_t removeHandler( IKeyboardHandler& h ) = 0;
};
   
}   
}} // namespaces
#endif
