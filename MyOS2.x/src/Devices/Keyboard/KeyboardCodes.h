/***************************************************************************
                          KeyboardCodes.h  -  description
                             -------------------
    begin                : Mon Aug 27 2001
    copyright            : (C) 2001 by jeroen
    email                : jeroen@thebem

    A convenient mapping of scancodes to enum values

    @note Special combos like CTRL-ALT-DEL must be checked manually, since the CTRL and ALT
          keys may be left or right or both: ((k&K_DELETE)==K_DELETE) && (k&K_CTRL)

    @note The keyboard interrupt routine will make the following conversions:
            ALT + 3 digits  -> ASCII code
            SHIFT + key     -> capital
            CTRL + <a..z>   -> ASCII code

 ***************************************************************************/
#ifndef KEYBOARDCODES_H
#define KEYBOARDCODES_H

#include "types.h"
#include "IKeyboardImpl.h"    // for E_KeyboardCode

namespace MyOS {
namespace Devices {
namespace Keyboard {

// helper class
class KeyCode
{
    u32 code;
public:
    inline KeyCode( u32 c ) : code(c) {}

    inline bool includesKey( E_KeyboardCode key ) const { return ((code&key)==key); }
    inline bool isKey( char key ) const {
      return includesKey( (E_KeyboardCode) (key&R_KEYMASK) );
    }
    inline bool isCtrlAltDelete() const {
        return includesKey(K_DEL) && (code&A_CTRL) && (code&A_ALT);
    }

    inline u32 getSpecialBits() const  { return code&R_SPECIALKEYS; }
    inline u32 getKeyBits() const      { return code&~R_SPECIALKEYS; }

    inline bool isCursorMove() const {
        return      ((R_CURSORMOVEMENT<=(code&R_KEYMASK))
                &&  ((code&R_KEYMASK) <=R_CURSORMOVEND));
    }

   /**
    * Interprets a scancode MAKE(!) code
    * based on current status of CAPS, SHIFTS, NUMLOCK & E_ESCAPE
    */
   static KeyCode interpret( u32 scancode8bit, u32 statusbits );

   operator u32() const { return code; }
   operator char() const { return code & 0xFF; }
};

}}}   // namespace

#endif
