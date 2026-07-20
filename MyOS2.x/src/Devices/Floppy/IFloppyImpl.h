#ifndef FloppyImpl_H
#define FloppyImpl_H

#include "Devices/Floppy/IFloppy.h"

#include "MM/IVirtualMemory.h"
#include "MM/IPhysicalMemory.h"  // for mapping lin->phys

// impl
// #include "FloppyThread.h"
#include "FloppyFSM.h"

namespace MyOS { namespace Devices { namespace Floppy {

   using Context::IContext;
   // using namespace MultiThreading;
   using namespace MM;

   class IFloppyImpl : public IFloppy {
public:
   IFloppyImpl( MyOS::Core::IComponent& c ) INITSECTION;

   bool init( ) INITSECTION;
   void deinit( );

   // XML
   virtual  myos_result_t getLabel( IO::OStream& out ) const;

   // IBlockDevice
   virtual myos_result_t open( );
   virtual void close( );
   virtual myos_result_t readBlocks( u32 lbnStart, u8 count, u8 seriesCount, IOCompletionHandler& h );
   virtual void releaseBuffer( buffer& buf );
   virtual void releasePhysicalBuffer( physadr_t b, size_t blockcount );   
   virtual myos_result_t writeBlocks( u32 lbnStart, size_t count, const buffer& data );
   virtual buffer getWriteBuffer( size_t blockcount ) 
      throw (Exceptions::OutOfBuffersException);
   virtual void releaseWriteBuffer( const buffer& buf );

private:
   // MultiThreading::Thread* worker;   
   // friend class FloppyThread;
   // FloppyThread runnable;
   
   FloppyFSM fsm;
};
   
}
}} // namespaces
#endif
