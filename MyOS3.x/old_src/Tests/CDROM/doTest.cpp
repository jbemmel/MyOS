/**
 * This file (same name) is included in every Tests/ subdirectory
 * By selecting proper directories in ant.include, this will cause 
 * one of the tests to be selected at all times
 */
#include "Init/MyOSCoreContainerDefs.h"
#ifndef IDEDriver_IN_CORE
#error This test needs the IDE driver
#endif 
  
#include "Context/IContext.h"
#include "debug.h"
#include "Tests/TestComponent.h"

#include "Devices/IDE/IIDEManager.h"
#include "Devices/IDE/IIDEDevice.h"

#include "FS/ISO9660/IISO9660FS.h"
#include "FS/IMountedFS.h"
#include "MultiThreading/IThread.h"

namespace MyOS { namespace Tests {

using namespace Devices::IDE;
using namespace Devices::Display;

using namespace MultiThreading;
using namespace FS;
using namespace FS::ISO9660;

/**
 * A test for the IDE driver + CDROM
 * 
 */
bool doTest( IContext& context, NVPAIR params[] )
{

   // Let's create a new thread, to make it possible to sleep...
   static class Runner : public IRunnable {
      virtual int run( NVPAIR params[] ) {

         IContext& context = * (IContext*) params[0].getValue();

   DPRINTK( "\nCDROM test running" );
   IInterface *r, *f;
   if (context.lookup( myos_name_t("9abd60a9-6b81-4d69-bb19-92f5e88de4cc"), r )==E_MYOS_SUCCESS) {
      IIDEManager* ideman = r->castTo<IIDEManager>();
      IIDEDevice* ata0;
      
     // XX on the laptop, the CD-ROM is MASTER on the _secondary_ controller!
     if (ideman->getDevice(2,ata0)) {
         
      if (context.lookup( myos_name_t("357cf5b1-0537-4164-ad4b-39896a5b0306"), f )==E_MYOS_SUCCESS) {
         IISO9660FS* isofs = f->castTo<IISO9660FS>();
         IMountedFS& fs = isofs->mount( *ata0 );
         fs.enumerateFiles( TestComponent::getInstance().getDisplay().cout() );

for (int i=0; i<1; ++i) {
         IOpenFile& kernel_bin = fs.openFilePaged( myos_name_t("README.TXT") );
         PRINTK( "\n[%d] File opened, size=%d", i, kernel_bin.getSize() );
         u8 data[32];
         memset_aligned( data, 0xCCCCCCCC, sizeof(data) );
         size_t count = kernel_bin.read( 0, sizeof(data), buffer( data,sizeof(data)) );
         PRINTK( "\nRead %d bytes at %x:", count, data );
         for (u32 b=0; b<sizeof(data); ++b) PRINTK( " %X", data[b] );
         kernel_bin.close();
}
         // fs.readFile( myos_name_t("readme.txt"), COUT );

         fs.unmount();  // TODO: This needs to wait for async operations...
         
         PRINTK( "\ntest Thread exits..." );
         return true;
      }      
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
