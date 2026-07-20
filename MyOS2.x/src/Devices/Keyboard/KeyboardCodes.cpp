/***************************************************************************
                          KeyboardCodes.cpp  -  description
                             -------------------
    begin                : Thu Sep 26 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#include "KeyboardCodes.h"

namespace MyOS {
namespace Devices {
namespace Keyboard {

// indirect table for control keys
static const u32 ctrlkey[] = {
   K_ESCAPE,
   K_BACKSPACE,
   K_TAB,      // could be done directly in scan2char table, but value is '\t'
   K_ENTER,    // could distinguish left/right too
   K_LCTRL,    // left, right only with escape code (0xe0)
   K_LSHIFT,
   K_RSHIFT,
   K_LALT,     // left
   S_CAPSLOCK | K_CAPSLOCK, // as key and as status
   K_F1,K_F2,K_F3,K_F4,K_F5,K_F6,K_F7,K_F8,K_F9,K_F10,
   S_NUMLOCK | K_NUMLOCK,
   S_SCROLLLOCK | K_SCROLLLOCK,
   (A_ALT | K_SYSREQ), // special case: <ALT> + <PRINTSCREEN>
   K_F11,K_F12,
};

// table for cursor movement, index by scancode-71
static const u32 cursorkey[] = {
   K_HOME,K_UP,K_PAGEUP,      // 7,8,9
   '-',
   K_LEFT,K_CENTER,K_RIGHT,   // 4,5(?),6
   '+',
   K_END,K_DOWN,K_PAGEDOWN,   // 1,2,3
   S_INSERT | K_INSERT,
   K_DEL,
};

// Translation of scancode to ASCII code, for KeyCodes < 255
static const char scan2char[] = {
   0xff,0,'1','2','3','4','5','6','7','8','9','0','-','=',1,  // 00 - 14
   2,'q','w','e','r','t','y','u','i','o','p','[',']',3,4,     // 15 - 29
   'a','s','d','f','g','h','j','k','l',';','\'','`',5,'\\',   // 30 - 43
   'z','x','c','v','b','n','m',',','.','/',                   // 44 - 53
   6,'*',7,   // shift R, * (printScreen?), alt
   ' ',
   8,9,10,11,12,13,14,15,16,17,18,19,20,     // CAPS, F1-F10,NUM,SCROLL

   // keypad, without numlock
   '7','8','9','-','4','5','6','+','1','2','3','0','.',

   // <ALT>+<PRINTSCREEN>, 2 invalid codes, F11 & F12
   21,0xff,0xff,22,23
};

// Characters when either <SHIFT> is pressed, CAPSLOCK normally only
// affects a-z
static const char scan2char_ucase[] = {
   0xff,0,'!','@','#','$','%','^','&','*','(',')','_','+',1,   // 00 - 14
   2,'Q','W','E','R','T','Y','U','I','O','P','{','}',3,4,      // 15 - 29
   'A','S','D','F','G','H','J','K','L',':','"','~',5,'|',      // 30 - 43
   'Z','X','C','V','B','N','M','<','>','?',                    // 44 - 53
   6,'*',7,   // shift R, * (printScreen?), alt
   ' ',
   8,9,10,11,12,13,14,15,16,17,18,19,20,     // CAPS, F1-F10,NUM,SCROLL

   // keypad, without numlock
   '7','8','9','-','4','5','6','+','1','2','3','0','.',

   // <ALT>+<PRINTSCREEN>, 2 invalid codes, F11 & F12
   21,0xff,0xff,22,23
};


// static
KeyCode
KeyCode::interpret( u32 sc, u32 statusbits )
{
   // PRINTF( "\ninterpret sc=%u status=%x", sc, statusbits );
   // register u32 key;
   #define key sc

   if (statusbits&A_SHIFT) {
      key = scan2char_ucase[sc];
      if ((statusbits&S_CAPSLOCK) && (key>='A'&&key<='Z')) {
        return KeyCode( key + ('a'-'A') );
      }        
   } else if ((sc>=71&&sc<=83) && (statusbits&(S_NUMLOCK|E_ESCAPE))) {
      return KeyCode( cursorkey[sc-71] );
   } else {
      key = scan2char[sc];
      if ((statusbits&S_CAPSLOCK) && (key>='a'&&key<='z')) {
         return KeyCode( key - ('a'-'A') );
      }        
   };

   // check for special keys
   if (key < (sizeof(ctrlkey)/sizeof(u32))) {
      key = ctrlkey[key];

      // check for right ctrl/alt, could make ctrlkey table twice its size and
      // index with ESCAPE added
      if ((statusbits&E_ESCAPE) && (key==K_LALT||key==K_LCTRL)) key>>=1;
   }

   return KeyCode(key);

   #undef key
}

}}}   // namespace


