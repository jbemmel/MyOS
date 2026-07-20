/**
 * This file (same name) is included in every Tests/ subdirectory
 * By selecting proper directories in ant.include, this will cause 
 * one of the tests to be selected at all times
 */
 
#include "Init/MyOSCoreContainerDefs.h"

/*
#ifndef Keyboard_IN_CORE
#error This test needs the keyboard handler
#endif
*/ 

 
#include "Context/IContext.h"
#include "Core/IScriptable.h"
#include "debug.h"

#include "Devices/Keyboard/IKeyboard.h"
#include "Devices/Display/IDisplay.h"
#include "Buffering/CBufInputStream.h"

namespace MyOS { namespace Tests {

using namespace Devices::Keyboard;
using namespace Devices::Display;

/**
 * A simple commandline interface, but quite powerful without too much effort!
 * 
 * Features I want to add:
 * - piping, eg IContext::enumerate | IXML::parse
 *   This highlight another issue: streams are currently implicitly (printable)
 *   character oriented, but you can have many different kinds of streams
 *   (more like flows of objects, e.g. XML trees)
 * 
 * - piping with XML-query filter (a la XPath), something like
 *   IContext::enumerate[ version > "1.0" ] or
 *   IContext::enumerate | IXML::select( "%/NODE" )
 * 
 * - Streaming with pre-allocation, like I had in previous MyOS versions:
 *   Obtain a put buffer, then do the put
 */
bool doTest( Context::IContext& context, NVPAIR params[] )
{
   // By putting this here, I make sure it is initialized!
   static class KeyboardHandler : public IKeyboardHandler
   {
      // To do lookup of interfaces (to call methods on them)
       Context::IContext& context;

      // Some misc interfaces
      IDisplay* display;
      
      char buffer[80];
      int cur;

      /**
       * Called when <ENTER> is pressed
       * 
       * NOTE: We are in IRQ context!
       */      
      void doCommand() {
         buffer[cur] = 0;
         DPRINTK( "\ndoCommand command=%s\n", buffer );
         if (strncmp(buffer,"cls",3)==0) {
            display->clear();   
         } else if ( char* s = strstr(buffer,"::") ) {
            *s = 0;  // terminate interface name
            try {
                IInterface &i = context.lookup( myos_name_t(buffer,s-buffer) );
    
               // TODO: Start a new thread to do this!
               // SOme interfaces cannot operator in IRQ contexts like this is
    
                Core::IScriptable *sc = i.getScriptable();
                if (sc) {
                    
                   NVPAIR params[] = { NVPAIR("",0) };
                   // XX Syntax should indicate get or put here...
                   if (sc->get( s+2, params, display->cout() )!=E_MYOS_SUCCESS) {
                      char* p = strchr( s+2, ' ');  // look for space
                      if (p) *p++ = 0;
                      else p = &buffer[cur];
                      IO::CBufInputStream in( (u8*) p, &buffer[cur] - p );
                      sc->put( s+2, params, in );
                   }
                 }
            } catch ( Context::InterfaceNotFoundException &nf ) {
                PRINTK( "Interface not found" );
            }
         } else if (cur>0) {  // ignore empty returns
            DPRINTK( "Unknown command\n" ); 
         }
                  
         cur = 0;
         PRINTK( "\nMyOS>" );
      }
      
   public:
      inline KeyboardHandler( Context::IContext& ctxt ) : context(ctxt), cur(0) {
         // TODO: Make display a device
         display = &ctxt.lookup( myos_name_t( IDisplay::ID() )).castToExcept<IDisplay>();
      }
   
      virtual void onKey( u32 key, u32 stickyStatus )
      {
         // DPRINTK( "\nonKey: %c", key );
         if (key=='\n') return doCommand();
         else if (cur<(int)sizeof(buffer)) {
           if (key==K_BACKSPACE || key==K_DEL) {
               if (cur>0) {
                  DPRINTK( "\b" );
                  buffer[--cur] = 0; 
               }                  
           } else {
              buffer[cur++] = (key&0xFF);
              DPRINTK( "%c", key ); // echo on commandline
           }              
         } else {
            DPRINTK( "\nToo many characters in command, clearing..." );
            buffer[ cur=0 ] = 0;
            DPRINTK( "\nMyOS>" );
         }
      }
   } handler( context );
      
   DPRINTK( "CommandLine test running" );
   IKeyboard& kb = context.lookup( myos_name_t("/dev/kbd0") ).castToExcept<IKeyboard>();
   kb.installHandler( handler );

   DPRINTK( "\nMyOS>" );
   return true;  
}

}}
