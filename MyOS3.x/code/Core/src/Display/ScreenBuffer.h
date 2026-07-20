/***************************************************************************
                          ScreenBuffer.h  -  description
                             -------------------
    begin                : Mon Apr 8 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef SCREENBUFFER_H
#define SCREENBUFFER_H

//#include "../build/gen_src/MyOSConfig.h"
//#ifdef CONFIG_IDisplay

#include "types.h"
#include "UUID.h"
#include "screentypes.h" // Color
#include "CPU/IOPort.h"
#include "string.h"     // strlen
#include "Name.h"       // myos_name_t

#include "Atomic/LockHelper.h"

namespace MyOS { namespace Devices { namespace Display {

/// Represents the textmode screenbuffer @ 0xB8000
class ScreenBuffer
{
   static ScreenBuffer* instance;
public:
   /// Screendimensions in characters
   enum E_SCREENDIMENSIONS { E_SCREENWIDTH=80, E_SCREENHEIGHT=25 };

   // for 90x60 hi-res mode
   // enum E_SCREENDIMENSIONS { E_SCREENWIDTH=90, E_SCREENHEIGHT=60 };

   typedef struct screenchar_t
   {
      char character    : 8;
      EColor attribute  : 8;

      screenchar_t( char c, EColor a ) : character(c), attribute(a) {}

      // used for filling below
      screenchar_t( u16 val ) : character( (char) val), attribute( (EColor) (val>>8) ) {}

   } PACKED SCREEN[E_SCREENHEIGHT][E_SCREENWIDTH];

public:
   inline static ScreenBuffer& getInstance() { return *instance; }

   /// A cursor represents a position on the actual screen
   class Cursor
   {
   public:
      /// Creates a cursor based on an existing cursor
      inline Cursor( const Cursor& other ) throw() : row(other.row), colom(other.colom) {}

      /// Prints text at the current cursor position, and advances the cursor
      Cursor& print( const char* text, size_t length, EColor color );

      /// Prints a single character at the current cursor position, and advances
      Cursor& print( char c, EColor color );

      /// Scrolls the display <count> lines
      void scrolllines( u8 count=1 );

      /// Advances the cursor 1 position
      inline Cursor& advance() {
         if (++colom >= E_SCREENWIDTH) { scrolllines(1); } return *this; }

      /// Moves the cursor 1 position back if possible
      inline Cursor& backspace() {
         if (colom>0) { --colom; }
         else if (row>0) { --row; colom=E_SCREENWIDTH; }
         return *this;
      }

      friend class Cursor& operator <<( Cursor &c, char ch );
      friend class Cursor& operator <<( Cursor &c, screenchar_t ch );

      /// as offset from ScreenBuffer[0][0] (upperleft corner)
      inline u32 getOffset() const { return colom + E_SCREENWIDTH*row; }

      /// Sets the position of this cursor as an offset from ScreenBuffer[0][0] (upperleft corner)
      inline bool setPosition( s32 offset ) {
         if (offset>=0 && offset<=E_SCREENHEIGHT*E_SCREENWIDTH) {
            colom = offset%E_SCREENWIDTH;
            row = offset/E_SCREENWIDTH;
            return true;
         } else return false;
      }

      /// Moves the cursor relative to the current position
      inline bool setPositionRelative( s32 offset ) {
         return setPosition( offset+row*E_SCREENWIDTH+colom );
      }

      inline void fill( u32 with, size_t count ) {
         ScreenBuffer::getInstance().fillfrom(row,colom,count,with);
         for (u32 c=count; c>0; --c) advance();
      }

   private:
      friend class ScreenBuffer;
      inline Cursor( u8 r, u8 c ) : row(r), colom(c) {}
      inline ~Cursor() throw() {}

      u8 row, colom;
   } PACKED;

   /// Creates a cursor at a specific location on the screen
   inline Cursor cursorAt( u32 row, u32 col ) { return Cursor(row,col); }

   /// Returns global cursor
   inline Cursor& cursor() { return globalCursor; }

   /// Returns a reference to the screenchar_t at the specified location, 0 if outside of screen
   inline screenchar_t* at( u32 row, u32 col )
   {
      return (row<E_SCREENHEIGHT && col<E_SCREENWIDTH) ? &((*BASE)[row][col]) : 0;
   }

   /// Writes a character to the screen, at given position and in given color
   inline bool put( u32 row, u32 col, char c, /* EColor */ u8 a )
   {
      if (screenchar_t* pos=at(row,col)) {
         pos->character = c;
         pos->attribute = (EColor) a;  // bitmask
         return true;
      }
      return false;
   }

   /// XORs a character on the screen with another character
   inline bool xorput( u32 row, u32 col, char c )
   {
      if (screenchar_t* pos=at(row,col)) {
         pos->character ^= c;
         // if (a!=E_BLACK) ((u8) pos->attribute) ^= a;
         return true;
      }
      return false;
   }

   /// Scrolls the screen down by n lines (default 1)
   void scrolldown( u8 lines = 1 );

   /// Clears the screen
   /// (with spaces and regular white-on-black attributes to see the cursor)
   inline void clear() {
       memset_aligned( BASE, 0x0F200F20, sizeof(SCREEN) );    // memset_aligned, but 2nd param is char!
       setCursor(0,0);
   }

   /// Clears a particular row on the screen (default with spaces), no check on row!
   inline void fillrow( u32 row, u32 with = 0x0F200F20 ) {
       memset_aligned( &(*BASE)[row][0], with, sizeof((*BASE)[0]) );
   }

   /// Prints characters starting from a particular position
   void fillfrom( u8 row, u8 col, size_t count, u32 with = 0x0F200F20 );

   /// Sets the position of the blinking cursor
   static void setCursor( u32 row, u32 col );

private:
   friend class Cursor& operator <<( Cursor &c, screenchar_t ch );

   friend class IDisplayImpl;
   ScreenBuffer( void* screenbase ) throw() INITSECTION;
   inline ~ScreenBuffer() throw() {}

   SCREEN* const BASE;
   Cursor globalCursor;

   spinlock_t screenlock;  // optional, to prevent distorted prints on screen
};

inline ScreenBuffer::Cursor&
operator <<( ScreenBuffer::Cursor &cursor, char c )
{
   ScreenBuffer::getInstance().put( cursor.row, cursor.colom, c, E_DEFAULT );
   return cursor.advance();
}

inline ScreenBuffer::Cursor&
operator <<( ScreenBuffer::Cursor &cursor, ScreenBuffer::screenchar_t ch )
{
   ScreenBuffer::getInstance().put( cursor.row, cursor.colom, ch.character, ch.attribute );
   return cursor.advance();
}

inline ScreenBuffer::Cursor&
operator <<( ScreenBuffer::Cursor &cursor, const char* msg )
{
   cursor.print( msg, strlen(msg), E_DEFAULT );
   return cursor;
}

inline ScreenBuffer::Cursor&
operator <<( ScreenBuffer::Cursor &cursor, const UUID& uuid )
{
   cursor.print( uuid.asString(), sizeof(uuid), E_DEFAULT );
   return cursor;
}

inline ScreenBuffer::Cursor&
operator <<( ScreenBuffer::Cursor &cursor, const myos_name_t& msg )
{
   cursor.print( msg, msg.length, E_DEFAULT );
   return cursor;
}

}}} // namespace

// #endif   // CONFIG_DISPLAY

#endif   // SCREENBUFFER_H
