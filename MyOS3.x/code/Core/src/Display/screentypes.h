#ifndef SCREENTYPES_H
#define SCREENTYPES_H

namespace MyOS { namespace Devices { namespace Display {

enum E_SCROLLOPTIONS {
    E_DOWN      = 0x0,
    E_UP        = 0x1,
};

/// Represents the colors used to display characters on the VGA screen (@todo use these)
enum EColor {
    E_BLACK         = 0,    // black is for both fg and bg
    E_BLUE          = 1,
    E_GREEN         = 2,
    E_CYAN          = 3,    
    E_RED           = 4,
    E_MAGENTA       = 5,
    E_BROWN         = 6,
    E_WHITE         = 7,
        
    /// Can be combined with one of the above, E_BRIGHT|E_BLACK == grey
    E_BRIGHT        = 0x8,
        
    E_YELLOW        = E_BROWN | E_BRIGHT,
        
    /// background colors, can be OR'ed with foreground
    E_BG_BLUE       = 0x10,
    E_BG_GREEN      = 0x20,
    E_BG_CYAN       = 0x30, 
    E_BG_RED        = 0x40,
    E_BG_MAGENTA    = 0x50,
    E_BG_BROWN      = 0x60,
    E_BG_WHITE      = 0x70,
        
    /// SOME COMBINATIONS
    E_DEFAULT = E_WHITE | E_BRIGHT,
    E_SUCCESS = E_DEFAULT | E_BG_GREEN,
    E_WARNING = E_DEFAULT | E_BG_BLUE,
    E_ERROR   = E_DEFAULT | E_BG_RED,
    
    /// Displays a character blinking in the foreground color
    E_BLINKING = 0X80
};

}}} // namespaces

#endif
