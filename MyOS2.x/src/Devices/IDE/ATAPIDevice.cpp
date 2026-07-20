#include "myosconfig.h"
#ifdef CONFIG_IDE

#include "ATAPIDevice.h"
#include "ATAPI.h"

namespace MyOS { namespace Devices { namespace IDE {

enum E_STATUS {
   E_IS_DATA    = (0<<0),
   E_IS_COMMAND = (1<<0),
   
   E_TO_DEVICE  = (0<<1),
   E_TO_HOST    = (1<<1),
   
   E_RELEASE    = (1<<2),  //< Device has released the bus, but command not done
};


ATAPIDevice::ATAPIDevice( IDEController& c, u8 masterSlave, u16 id[256] )
: IIDEDeviceImpl(c,masterSlave)
{
   DPRINTK( "\nNew ATAPI device masterSlave=%b", masterSlave ); 
   
   // TODO: Extract useful data from 'id' info
}
 
// virtual 
myos_result_t
ATAPIDevice::getVolumeLabel( IO::OStream& out ) const {
   return E_MYOS_E_NOTIMPL;
}

//virtual 
myos_result_t 
ATAPIDevice::open( )
{
   return E_MYOS_SUCCESS;  
}

//virtual 
void 
ATAPIDevice::close( )
{
   
}

// inline methods
inline void*
IORequest::operator new( size_t s )
{
   return ::operator new<MyOS::Drivers::IDE::IDEDriverComponent,true>( s );   
}

inline void
IORequest::operator delete( void* r )
{
   ::operator delete<MyOS::Drivers::IDE::IDEDriverComponent>( r, sizeof(IORequest) );   
}
   
//virtual 
myos_result_t 
ATAPIDevice::readBlocks( u32 lbnStart, u8 count, u8 seriesCount, IOCompletionHandler& h )
{
   DPRINTK( "\nATAPIDevice::readBlocks s=%u c=%X", lbnStart, count ); 
   
   IORequest* rq = new IORequest( lbnStart, count, seriesCount, h );
   if (controller.handleNow(*rq)) {
      this->process(*rq);     
   } else {
      DPRINTK( "\nEnqueuing request, controller not ready..." );
      requests.enqueue( *rq );
   }   
   return E_MYOS_SUCCESS;      
}

//virtual 
void 
ATAPIDevice::process( IORequest& rq )
{
   // XX this always assumes it's a read request, which needs SEEK!
   ATAPICommandPacket cmd( E_SEEK, rq.getStartBlock() );
   
   // Could choose multiple blockcount...
   controller.setSeeking(masterSlave);   
   controller.sendATAPICommand( cmd, 0, masterSlave );   
}

// virtual 
void 
ATAPIDevice::onSeekDone( IORequest& rq )
{
   DPRINTK( "\nATAPIDevice::onSeekDone" );   
   
   ATAPICommandPacket cmd( E_READ_EXT, rq.getStartBlock(), rq.getCount() );
   
   // Could choose multiple blockcount...
   controller.setReading(masterSlave);
   controller.sendATAPICommand( cmd, rq.getCount() * 2048, masterSlave );   
}


// virtual 
void
ATAPIDevice::onReadDone( IORequest& rq )
{
   DPRINTK( "\nATAPIDevice::onReadDone" );
   
   // Use dynamic allocation here? Only for PIO, for DMA it must already be done
   // Perhaps better to allocate before issueing 'read' command
   u16* page = (u16*) IDEDriverComponent::getInstance().iVirtualMemory->allocPage( rq.getAddr() );
   size_t count = controller.readData( page, _4KB );

   // reuse count variable   
   count = rq.onRead( buffer((u8*)page,count) );
   if ((count&IOCompletionHandler::E_KEEPBUFFER)==0) {
      IDEDriverComponent::getInstance().iVirtualMemory->freePage( page );
   }
   if ((count&IOCompletionHandler::E_REISSUE)==0) {
      delete &rq;    // == controller.curRq, which will be reset later
   } else {
      rq.reissued();

      // To ensure fairness and prevent starvation, put it back in line
      // even is request queue is empty (other device can be next)
      requests.enqueue( rq );   // reissue, modified
   }
}

//virtual 
void 
ATAPIDevice::releaseBuffer( buffer& buf )
{
   
}

//virtual 
void 
ATAPIDevice::releasePhysicalBuffer( MM::physadr_t b, size_t blockcount )
{
   
}

// not yet virtual, not implemented by Bochs
void 
ATAPIDevice::playAudio()
{
   ATAPICommandPacket cmd( E_PLAY_AUDIO, 0, 1 );
   controller.sendATAPICommand( cmd, 0, masterSlave );
}
   
}}}   // namespace

#endif
