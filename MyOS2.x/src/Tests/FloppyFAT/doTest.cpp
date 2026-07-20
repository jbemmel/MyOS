/**
 * This file (same name) is included in every Tests/ subdirectory
 * By selecting proper directories in ant.include, this will cause 
 * one of the tests to be selected at all times
 */
#include "Init/MyOSCoreContainerDefs.h"
#ifndef Floppy_IN_CORE
#error This test needs the floppy driver
#endif 
#ifndef FAT_IN_CORE
#error This test needs the FAT FS driver
#endif 
 
 
#include "Context/IContext.h"
#include "debug.h"
#include "Tests/TestComponent.h"

#include "Devices/Floppy/IFloppy.h"
#include "FS/FAT/IFATFS.h"
#include "FS/IOpenFile.h"

#include "MultiThreading/IThread.h"

namespace MyOS { namespace Tests {

using namespace Devices::Floppy;
using namespace FS::FAT;
using namespace FS;
using namespace Devices::Display;

using namespace MultiThreading;

/**
 * A test for the floppy driver + FAT FS
 * 
 */
bool doTest( IContext& context, NVPAIR params[] )
{
   /* context == BootContext! Switch to real impl
   IContext* real;
   if (context.lookup( myos_name_t("bdf79a36-5871-11d6-858d-0010a708e02e"), (IInterface*&) real ) != E_MYOS_SUCCESS) {
      DPRINTK( "\n##Could not find real context!" );
      return false;
   }
   */

   // Let's create a new thread, to make it possible to sleep...
   static class Runner : public IRunnable {
      virtual int run( NVPAIR params[] ) {

         IContext& context = * (IContext*) params[0].getValue();

   DPRINTK( "\nFloppyFAT test running" );
   IInterface *r, *f;
   
//   {
//        MyOS::Devices::LPT::LPTStream printer;
//        context.enumerate( printer );
//   }        
   if (context.lookup( myos_name_t("/dev/fdd0"), r )==E_MYOS_SUCCESS) {
      IFloppy* floppy = r->castTo<IFloppy>();
      
      if (context.lookup( IFATFS::uuid(), f )==E_MYOS_SUCCESS) {
         IFATFS* fatfs = f->castTo<IFATFS>();
         IMountedFS& fs = fatfs->mount( *floppy );
         fs.enumerateFiles( TestComponent::getInstance().getDisplay().cout() );
/*
for (int i=0; i<1; ++i) {
         IOpenFile& kernel_bin = fs.openFilePaged( myos_name_t("KERNEL.BIN") );
         PRINTK( "\n[%d] File opened, size=%d", i, kernel_bin.getSize() );
         u8 data[32];
         memset_aligned( data, 0xCCCCCCCC, sizeof(data) );
         size_t count = kernel_bin.read( 0, sizeof(data), buffer( data,sizeof(data)) );
         PRINTK( "\nRead %d bytes at %x:", count, data );
         for (u32 b=0; b<sizeof(data); ++b) PRINTK( " %X", data[b] );
         kernel_bin.close();
}
         // fatfs->readFile( myos_name_t("hello.wld"), COUT );
*/

#ifdef DEBUG         
         MyOS::Devices::LPT::LPTStream printer;
         try {
            fs.readFile( myos_name_t("test/test.txt"), /*COUT*/ printer );
         } catch (FileNotFoundException &fnfe) {
            DPRINTK( "\nFileNotFound exception caught" );  
         }            

         // XX Dont exit thread, TODO: solve this: Check for pending operations
         // before deleting a thread
         // while (1);
#endif

         fs.unmount();  // TODO: This needs to wait for async operations...
         
         PRINTK( "\ntest Thread exits..." );
         return true;
      }
   }
         
         return 0;
      }      
   } sleeper;      

   Thread& t = TestComponent::getInstance().getMT().createThread(0);
   NVPAIR ps[] = { NVPAIR("context", (const char*) &context), NVPAIR("",0) };
   t.start( sleeper, ps );   
   
   // dont let sleeper get destroyed! -> static ? doesn't work enough
   while (1);

   return false;
}

}}
