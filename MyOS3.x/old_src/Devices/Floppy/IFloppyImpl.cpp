#include "IFloppyImpl.h"
#include "Drivers/Floppy/Default/FloppyDriver.h"

namespace MyOS { namespace Devices { namespace Floppy {

using MyOS::Drivers::Floppy::Default::FloppyDriver;

IFloppyImpl::IFloppyImpl( MyOS::Core::IComponent& c )
: IFloppy( c, VERSION(1,0) ), fsm()
{

}

bool 
IFloppyImpl::init( ) /* INITSECTION */
{
 
   // worker = &FloppyComponent::getInstance().iMultiThreading->createThread(0);
   // runnable.init( *worker );
   FloppyDriver::getInstance().iInterruptHandling->setIRQHandler( E_IRQ6, fsm, E_ENABLE_INTS );

   //NVPAIR ps[] = { NVPAIR("this",(const char*)this), NVPAIR(0,0) };
   //worker->start( runnable, ps );
	return fsm.init();
}

void 
IFloppyImpl::deinit( )
{
   FloppyDriver::getInstance().iInterruptHandling->removeIRQHandler( E_IRQ6 );
   // worker = 0;

   // thread deletes itself, try sleep to let it run ?
   // runnable.deinit();
}

// virtual 
myos_result_t
IFloppyImpl::getLabel( IO::OStream& out ) const {
   return E_MYOS_E_NOTIMPL;
}

// virtual 
myos_result_t IFloppyImpl::open( )
{
   return E_MYOS_SUCCESS;
}

// virtual 
void IFloppyImpl::close(  )
{
   
}

// virtual 
myos_result_t IFloppyImpl::readBlocks( u32 lbnStart, u8 count, u8 seriesCount, IOCompletionHandler& h )
{
   DPRINTK( "\nreadBlocks LBN=%d count=%d", lbnStart, count ); 

   // "Unlimited" queue, could restrict...
   fsm.addRequest( lbnStart, count, seriesCount, h ); 
   return E_MYOS_SUCCESS;
}

//virtual 
void 
IFloppyImpl::releaseBuffer( buffer& buf )
{
   fsm.releaseBuffer(buf); 
}

// virtual 
void 
IFloppyImpl::releasePhysicalBuffer( physadr_t b, size_t blockcount )
{
   fsm.releasePhysicalBuffer(b,blockcount);  
}

// virtual 
myos_result_t IFloppyImpl::writeBlocks( u32 lbnStart, size_t count, const buffer& data )
{
   return E_MYOS_ERROR;
}

// virtual 
buffer IFloppyImpl::getWriteBuffer( size_t blockcount ) 
      throw (Exceptions::OutOfBuffersException)
{
   // TODO...
   throw Exceptions::OutOfBuffersException(); 
}

// virtual 
void IFloppyImpl::releaseWriteBuffer( const buffer& buf )
{
 
}

}}}  // namespaces
