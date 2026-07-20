/***************************************************************************
                          ScreenBuffer.cpp  -  description
                             -------------------
    begin                : Mon Apr 8 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#include "MyOSConfig.h"
#ifdef CONFIG_IDisplay

#include "ScreenBuffer.h"
#include "string.h"
#include "IO/IOUtil.h"
#include "defs.h"  // INITINSTANCE

#include "debug.h"

namespace MyOS { namespace Devices { namespace Display {
   
ScreenBuffer* ScreenBuffer::instance INITINSTANCE;

ScreenBuffer::ScreenBuffer( void* screenbase )
   : BASE( (SCREEN*) screenbase), globalCursor(0,0)
{
   spin_lock_init( screenlock );
   instance = this;
   clear();

#ifdef DEBUG_IRQS
   // for debugging of IRQ occurance, paint '0' on the screen
   fillrow( 23, 0x07300730 );
#endif
}

ScreenBuffer::Cursor&
ScreenBuffer::Cursor::print( const char *src, size_t length, EColor att )
{
	// Danger! This may cause a loop, this function is called from assert() !
	ASSERTION( (src!=0), E_ERROR );

	// put a reasonable limit to catch bogus sizes
	ASSERTIONv( length <= 80*25, E_ERROR, length );

	// Protect this code...
	// XX This lock should be recursive! In case of page faults...
	Locking::LockHelper locked( ScreenBuffer::getInstance().screenlock );
	while (length--)
	{
		switch (char c = *src++)
		{
      // these are here to force generation of jump table
      case 0xb:
      case 0xc:
      case '\r':
         break;   // ignore   (could ignore all non-printable)

      case '\b':
         backspace();
         ScreenBuffer::getInstance().put( row, colom, ' ', att );
         break;

      case '\t':
		{
			s8 tabstop = colom & 0x7;
         // spaces with att as background
         fill( ((att|(att<<16))<<8) | 0x00200020, tabstop ? (8-tabstop) : 8 );
         break;
		}

      case '\n':
		    scrolllines(1);
		    break;

      // interpret '#xx#' as hex attribute change
      case '#':
         if (length>2 && src[2]=='#')   {
            att = (EColor) ((IO::IOUtil::toU8(src[0])<<4) + IO::IOUtil::toU8(src[1]));
            src+=3;
            length-=3;
            break;
         } // else default

		default:    // could write per 32bit
         ScreenBuffer::getInstance().put( row, colom, c, att );

			// wrap around bottom of screen
			advance();
		}
	}

	// update blinking cursor on screen
	ScreenBuffer::getInstance().setCursor( row, colom );
	return *this;
}

ScreenBuffer::Cursor& 
ScreenBuffer::Cursor::print( char c, EColor att )
{
	Locking::LockHelper locked( ScreenBuffer::getInstance().screenlock );
	ScreenBuffer::getInstance().put( row, colom, c, att );
	advance();
	return *this;
}

void
ScreenBuffer::Cursor::scrolllines( u8 lines /* =1 */ )
{
#ifdef DEBUG_IRQS
   if (++row >= E_SCREENHEIGHT-2) {
      ScreenBuffer::getInstance().scrolldown(lines);
      row = E_SCREENHEIGHT-3;
   }
#else
   /// Keep 1 row above bottom row, otherwise wrapping text is cleared!
   if (++row >= E_SCREENHEIGHT-1) {
      ScreenBuffer::getInstance().scrolldown(lines);
      row = E_SCREENHEIGHT-2;
   }
#endif
   colom = 0;	// do this last, scroll() tends to get interrupted by another thread...
}

/// This method assumes the bottomline of the screen is used for something else...
void ScreenBuffer::scrolldown( u8 lines /* =1 */) {
#ifdef DEBUG_IRQS
	memcpy_aligned( BASE, (*BASE)[lines], (E_SCREENHEIGHT-2-lines) * E_SCREENWIDTH * 2 );
    fillrow( E_SCREENHEIGHT-2-lines );
#else
	memcpy_aligned( BASE, (*BASE)[lines], (E_SCREENHEIGHT-1-lines) * E_SCREENWIDTH * 2 );
	fillrow( E_SCREENHEIGHT-1-lines );
#endif
}

// static
void
ScreenBuffer::setCursor(u32 row, u32 col)
{
   enum { E_CURSOR_LOW=0x0F, E_CURSOR_HI=0x0E };   // commands
   u32 position = (row*E_SCREENWIDTH) + col;

   // DPRINTP( "\nrow=%d col=%d setCursor->%d", row, col, position );

   // ports for manipulating the cursor
   IOPort8<0x3D4> VGA_INDEX;
   IOPort8<0x3D5> VGA_DATA;

   // cursor LOW port to vga INDEX register
   VGA_INDEX = E_CURSOR_LOW;
   VGA_DATA = position&0xFF;
   // cursor HIGH port to vga INDEX register
   VGA_INDEX = E_CURSOR_HI;
   VGA_DATA = (position>>8)&0xFF;
}

/// Prints characters starting from a particular position
void
ScreenBuffer::fillfrom( u8 row, u8 col, size_t count, u32 with ) {

   // take care of misaligned bytes
   if (col&1) {
      (*BASE)[row][col++] = with;
      --count;
   }

   // pattern must be 2 bytes & include proper bg bits
   memset_aligned( &(*BASE)[row][col], with, 2*count );
}

}}} // namespace

#endif
