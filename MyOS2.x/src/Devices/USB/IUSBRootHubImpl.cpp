/**
 * UHCI driver (for now only Via)
 * 
 * @see http://www.alsa-project.org/alsa/ftp/datasheets/via/686a.pdf for info
 *
 * @see http://cvs.sourceforge.net/viewcvs.py/linux-lc100020/ for USB WLAN driver
 * @see ftp://download.intel.com/technology/usb/UHCI11D.PDF
 */

#include "IUSBRootHubImpl.h"
#include "Drivers/USB/Standard/UHCIDriver.h"
#include "MM/new.h"

#include "Drivers/PCI/IPCIDevice.h"
#include "Drivers/PCI/PCIConfig.h"
#include "Drivers/PCI/IPCISupport.h"

#include "IUSBDeviceImpl.h"

// temp
#include "Timer/TimeUtil.h"

namespace MyOS { namespace Devices { namespace USB {

using namespace Drivers::USB::Standard;

#ifdef __GNUC__
inline void*
IUSBRootHubImpl::operator new( size_t s )
{
   return allocate( s );   
}

inline void
IUSBRootHubImpl::operator delete( void* r )
{
   deallocate( r, sizeof(IUSBRootHubImpl) );   
}

inline void*
IUSBDeviceImpl::operator new( size_t s )
{
   return allocate( s );   
}

inline void
IUSBDeviceImpl::operator delete( void* r )
{
   deallocate( r, sizeof(IUSBDeviceImpl) );   
}

#endif

IUSBRootHubImpl::IUSBRootHubImpl( MyOS::Core::IComponent& c, IPCIDevice& d )
: IUSBRootHub( c, VERSION(1,0) ), dev(d)
{
    // reserve address 0
    addresses.clear();
    addresses.markUsed(0);
    
    data_tds.clear();
}

bool 
IUSBRootHubImpl::init( IDriverManager& dm ) /* INITSECTION */
{
	// Linux first disables legacy support, Via documentation says it's read-only...
	// legacySupport.write( dev.getInfo().handle, 0 );
    // DPRINTK( "\nLegacy support (R/O?): %x", legacySupport.read( dev.getInfo().handle ) );
    
	// Note: not very clean, perhaps define interface...
    USBBasePort = PCICONFIG_FIXED.type0.base_address4.read( dev.getInfo().handle ) & ~1;
    DPRINTK( "\nUSB I/O base port: %x", USBBasePort );

    port1.init( USBBasePort + E_PORT1SC );
    port2.init( USBBasePort + E_PORT2SC );

    waitpoint = 1;

    // Create a separate thread, to make sleeping work       
    this->thread = &UHCIDriver::getInstance().iMultiThreading->createThread(0);
    NVPAIR params[] = { NVPAIR("dm",&dm) };
    thread->start( *this, params, sizeof(params)/sizeof(NVPAIR) );

    // need to wait for thread, init cannot sleep...
    while (waitpoint);

    // Need to reset the ports? Why, controller just did a reset.. yes, needed   
    port1.reset(dm);
    u16 status; 
    if ((status = port1.detectAndEnable(dm))) {
        DPRINTK( "\nDetected USB device on port 1" );
        IUSBDeviceImpl* dev = new IUSBDeviceImpl( *this, addresses.getIndex(), status & E_IS_LOW_SPEED );
        port1.setDevice( *dev );
    }
    port2.reset(dm);    
    if ((status = port2.detectAndEnable(dm))) {
        DPRINTK( "\nDetected USB device on port 2" );       
        IUSBDeviceImpl* dev = new IUSBDeviceImpl( *this, addresses.getIndex(), status & E_IS_LOW_SPEED );
        port2.setDevice( *dev );
    }
        
    DPRINTK( "\nPort[0]:%x Port[1]:%x", readPort(0), readPort(1) );    
    return true;
}

// virtual 
int 
IUSBRootHubImpl::run( NVPAIR params[] )
{	
    IDriverManager* dm = (IDriverManager*) params[0].getValue();
 
    // see Linux http://lxr.linux.no/source/drivers/usb/host/uhci-hcd.c
    // This is a HARD reset
//    writeCommand( E_HCRESET );	
//    dm->usDelay( 50 * 1000 );
//    writeCommand( E_ZERO );
//    dm->usDelay( 10 * 1000 );

    // see Linux http://lxr.linux.no/source/drivers/usb/host/uhci-hcd.c#L2080
    int timeout = 1000;    
    writeCommand( E_HCRESET );
	while ( readCommand() & E_HCRESET )
	{
		if (--timeout==0) {
			DPRINTK( "\nTimeout while resetting UHCI Controller" );
			return false;
		}
	}

    // Allocate 1 page of DMA memory, but do *not* use allocDMA !
    framelist = (USB_FLP*) UHCIDriver::getInstance().iVirtualMemory->allocPage( dmaMemory );
	DPRINTK( "\nUSB framelist allocated at virt=%x phys=%x", framelist, dmaMemory );

    // Allocate 1 page of DMA memory for data TDs, but do *not* use allocDMA !
    tddata = (USB_TD*) UHCIDriver::getInstance().iVirtualMemory->allocPage( dmaTddata );

    // set whole list to 'invalid pointer'
    memset_aligned( framelist, 0x1, _4KB /* sizeof(MM::Page) */ );

	dm->getPCISupport().enableIRQ( dev, *this );

	enableInterrupts( 0x000F );		// enable all interrupts, bits 0..3
	setFrame( 0 );
	setFrameListBase( dmaMemory );
	writeCommand( E_RUN | E_CFG_DONE | E_MAXPKT64 );

	// re-enable legacy support
	// legacySupport.write( dev.getInfo().handle, 0x2000 );

	DPRINTK( "\nUHCI controller running, status %x MISC1=%X MISC2=%X", 
        readStatus(), 
        PCICONFIG_FIXED.type0.misc_control_1.read(dev.getInfo().handle),
        PCICONFIG_FIXED.type0.misc_control_2.read(dev.getInfo().handle)
    );
    waitpoint = 0;    
    while (thread)
    {
        if (!thread->usSleep( 10 * 1000 * 1000 )) {     // make this indefinite?
            DPRINTK( "\nRoot hub thread awakened by IRQ" );
            sync.signal();
        }            
    }
    return 0;
}

void
IUSBRootHubImpl::deinit( IContext& context )
{

}

// virtual 
void
IUSBRootHubImpl::onInterrupt( )
{
    CHECKPOINT('u');
	u16 status = readStatus();
	if (status & ~E_HALTED) {
        clearStatus( status );  // right away     
        if (status!=E_USBINT) {
			if (status & E_HALTED) {
				writeCommand( readCommand() | E_RUN );	// try to restart it
			}
        }

		// TODO: Process URBs here
		DPRINTK( "\nstatus=%x port0=%x curIndex=%x", status, readPort(0), curFrame() );

        // see uhci1.1 spec page 22 IOC bit: if set, irq is generated even if 
        // no data is actually send on the USB bus. So clear it to avoid repeats
        if (tdhack) {
          tdhack->control &= ~1;
          tdhack = 0;
        }          

	}	// else shared interrupt -> do nothing

    // return this->thread;    // always ?
}

bool 
IUSBRootHubImpl::schedule( USB_CONTROL_PACKET& ctrl, IUSBDeviceImpl& d )
{
    register int statctrl = d.isLowSpeed() ? (0x04000000 | 0x18800000) : 0x18800000;
    USB_TD setup = {
        0, 
        statctrl /*{0,0,0x80,0x00}*/, 
        PH( E_SETUP, d.getAddress(), 0, 0, sizeof(USB_CONTROL_PACKET)-1), 
        UHCIDriver::getInstance().iPhysicalMemory->getPhysicalAddress( &ctrl )
    };
    USB_TD status = {
        0x1,    // isLast
        statctrl | 0x01000000 /* set IOC bit */, 
        PH( E_IN, d.getAddress(), 0, 1, E_NULL_DATA), 
        0
    }; 
    
    // They need to be in a queue, else the data/status transfer fails!
    USB_QH qh = { 0x1, UHCIDriver::getInstance().iPhysicalMemory->getPhysicalAddress( &setup ) };
      
    ASSERTIONv( (((u32) &setup) & 0xF)==0, E_CRITICAL, &setup );
    setup.next = UHCIDriver::getInstance().iPhysicalMemory->getPhysicalAddress( &status );        

    // !!! HACK !!!
    tdhack = &status;
    // !!! END HACK !!!

    register int retries = 3;
    u32 qhPhys = UHCIDriver::getInstance().iPhysicalMemory->getPhysicalAddress( &qh );
retry:    
    u16 index = (curFrame() + 2) & 1023;
    // TODO: atomic
    ((u32*)framelist)[ index ] = 0x2 | qhPhys;

    DPRINTK( "\nURB scheduled at index %d statctrl=%x", index, statctrl );
    // return sync.wait( 2000 );
    Timer::TimeUtil::delay<Timer::ms>( 2500 );	// XX busy wait
    ((u32*)framelist)[ index ] = 0x1;
    
    DPRINTK( "\nsetup:res=%x %d setup:stat=%X status:stat=%X %d", 
        setup.result, setup.actualLen, setup.status,
        status.status, status.actualLen
    );
    if (setup.status != 0 && (--retries > 0)) {
        setup.result = statctrl;
        goto retry;
    } else if (status.status != 0 && (--retries > 0)) {
        setup.result = statctrl;    // clear
        status.result = statctrl | 0x01000000;
        goto retry;      
    }
    DPRINTK( "..OK!" );
    tdhack = 0;
    return true;
}

bool 
IUSBRootHubImpl::schedule( USB_CONTROL_PACKET& ctrl, IUSBDeviceImpl& d, void* data, size_t dataSize )
{
    // can use 0x18000000 to set max_errors=3
    register int statctrl = d.isLowSpeed() ? (0x04000000 | 0x18800000) : 0x18800000;
    USB_TD setup = {
        0, 
        statctrl /*{0,0,0x80,0x00}*/, 
        PH( E_SETUP, d.getAddress(), 0, 0, sizeof(USB_CONTROL_PACKET)-1), 
        UHCIDriver::getInstance().iPhysicalMemory->getPhysicalAddress( &ctrl )
    };
/*
    USB_TD tdData = { // max. 2 packet for now, toggle=1
        0,
        statctrl | 0x18800000, E_IN, d.getAddress(), 0, 1, 0, 8, 
        UHCIDriver::getInstance().iPhysicalMemory->getPhysicalAddress( data )
    };     
    USB_TD tdData2 = { // max. 2 packet for now, toggle=0
        0,
        statctrl | 0x18800000, E_IN, d.getAddress(), 0, 0, 0, 8, 
        UHCIDriver::getInstance().iPhysicalMemory->getPhysicalAddress( data )+8
    };         
*/    
    USB_TD status = {
        0x1,    // isLast
        statctrl | 0x01000000 /* IOC bit set */, 
        PH( E_OUT, d.getAddress(), 0, 1, E_NULL_DATA ), 
        0
    }; 
    
    // They need to be in a queue, else the data transfer fails!
    USB_QH qh = { 0x1, UHCIDriver::getInstance().iPhysicalMemory->getPhysicalAddress( &setup ) };
    ASSERTIONv( (((u32) &setup) & 0xF)==0, E_CRITICAL, &setup );

    MM::physadr_t dp = UHCIDriver::getInstance().iPhysicalMemory->getPhysicalAddress( data );
        
    u32 maxsize = d.getMaxControlPacketSize();
    u8 packets = (maxsize + dataSize - 1) / maxsize;   // calc #packets

    // for now, as one block (easier for deallocation)
    u8 index = data_tds.getMulti( packets );
    USB_TD *td = &tddata[ index ];
    setup.next = dmaTddata | ((u32)(td) & 0xFFF);

    // do all but the last data packet
    for (int p = 1; p<packets; ++p, ++td) {
        // write all fields -> no zeroing needed
        td->next = dmaTddata | ((u32)(td+1) & 0xFFF);
        td->result = statctrl;
        td->ph = PH(E_IN,d.getAddress(),0, (p&1), maxsize-1 );
        td->dataBuffer = dp + (p-1) * maxsize;
        
        dataSize -= maxsize;
    }
    // fix last element
    tddata[ index + packets - 1 ].next = UHCIDriver::getInstance().iPhysicalMemory->getPhysicalAddress( &status );
    tddata[ index + packets - 1 ].result = statctrl;
    tddata[ index + packets - 1 ].ph = PH(E_IN,d.getAddress(),0, (packets&1), dataSize-1 );
    tddata[ index + packets - 1 ].dataBuffer = dp + (packets-1) * maxsize;
        
    // !!! HACK !!!
    tdhack = &status;
    // !!! END HACK !!!

    register int retries = 3;
    u32 qhPhys = UHCIDriver::getInstance().iPhysicalMemory->getPhysicalAddress( &qh );
retry:    
    u16 frame = (curFrame() + 2) & 1023;
    // TODO: atomic
    ((u32*)framelist)[ frame ] = 0x2 | qhPhys;

    DPRINTK( "\nURB scheduled at index %d statctrl=%x", frame, statctrl );
    // return sync.wait( 2000 );
    Timer::TimeUtil::delay<Timer::ms>( 2500 );		// XX busy wait, fix
    ((u32*)framelist)[ frame ] = 0x1;
    
    DPRINTK( "\nsetup:res=%x %d status:stat=%X %d", 
        setup.result, setup.actualLen, 
        status.status, status.actualLen
    );
    if (setup.status != 0 && --retries > 0) {
        setup.result = statctrl;
        goto retry;
    } else if (status.status != 0 && --retries > 0) {
        setup.result = statctrl;    // clear
        status.result = statctrl | 0x01000000;
        goto retry;      
    }
    
    data_tds.releaseMulti( index, packets );
    
    tdhack = 0;
    return true;
}

}}}  // namespaces
