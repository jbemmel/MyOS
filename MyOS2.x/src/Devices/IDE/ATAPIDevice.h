#ifndef ATAPIDEVICE_H
#define ATAPIDEVICE_H

#include "IIDEDeviceImpl.h"
#include "MM/new.h"
#include "Drivers/IDE/IDEDriverComponent.h"

namespace MyOS { namespace Devices { namespace IDE {

using MyOS::Drivers::IDE::IDEDriverComponent;
   
class ATAPIDevice : public IIDEDeviceImpl
{   
public:
   inline void* operator new( size_t s ) {
      return ::operator new<IDEDriverComponent>(s);
   }
   inline void operator delete( void* p ) {
      ::operator delete<IDEDriverComponent>( p, sizeof(ATAPIDevice) );
   }
   
   ATAPIDevice( IDEController& controller, u8 masterSlave, u16 id[256] );

   // XML
   virtual myos_result_t getVolumeLabel( IO::OStream& out ) const;

   /// IBlockDevice
   virtual myos_result_t open( );
   virtual void close( );
   virtual myos_result_t readBlocks( u32 lbnStart, u8 count, u8 seriesCount, IOCompletionHandler& h );
   virtual void releaseBuffer( buffer& buf );
   virtual void releasePhysicalBuffer( MM::physadr_t b, size_t blockcount );

   // Controller
   virtual void onSeekDone( IORequest& rq );
   virtual void onReadDone( IORequest& rq );
   virtual void process( IORequest& rq );
   
   // XX These should be in XML interface!
   void playAudio();
};   
   
}}}   // namespace

#endif
