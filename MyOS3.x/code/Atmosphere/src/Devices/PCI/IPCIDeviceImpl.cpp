#include "IPCIDeviceImpl.h"
#include "../DriverManagerComponent.h"
#include "new.h"

#include "debug.h"

namespace MyOS { namespace Drivers { namespace PCI {

IPCIDeviceImpl::IPCIDeviceImpl( E_PCI_ID vendor, PCIDeviceHandle h )
: IPCIDevice( DriverManagerComponent::getInstance(), VERSION(1,0) )
, info(h), header_type(0), ringnode(*this)
{
   /* assign an ID: done in IDriverManagerImpl::registerDevice
   static int nextID = 0;
   strcpy( info.systemID, "pci00" );
   s32 myID = nextID++;
   info.systemID[3] += (myID/10);
   info.systemID[4] += (myID%10);
   */

   ASSERTIONv(h & 0x80000000, E_CRITICAL, h );
   {
      // SimpleLock l; SimpleLock::PROTECTED region(l);   

        info.id.vendor = vendor;    
        info.id.device = PCICONFIG_FIXED.device.read( h );
        info.classID = PCICONFIG_FIXED.device_class.read( h );
        info.revision = PCICONFIG_FIXED.revision.read( h );
        header_type = PCICONFIG_FIXED.header_type.read( h );

        DPRINTK( "\nPCI device: vendor %x id %x", (u16) vendor, (u16) info.id.device );

        if (header_type.type==PCI_HEADER_TYPE::TYPE0) {
            u8 irq = PCICONFIG_FIXED.type0.interrupt_pin.read( h );
            if (irq) {
                info.assignedIRQ = PCICONFIG_FIXED.type0.interrupt_irq.read( h );
            }
                           
            // which addresses are stored where ? I guess it depends on the device...
            for (int i=0; i<6; ++i) {           
                PCI_MEM_INFO base =  PCICONFIG_FIXED.base_address0.readOffset( h, 4*i );
                if (base.getAddress())
                {
                    PCICONFIG_FIXED.base_address0.writeOffset( h, 4*i, ~0 );
                    PCI_MEM_INFO size = PCICONFIG_FIXED.base_address0.readOffset( h, 4*i );
                    u32 decoded_size = size.getAddress();
                    decoded_size &= ~(decoded_size-1);  // decode size, Linux pci.c
                    PCICONFIG_FIXED.base_address0.writeOffset( h, 4*i, base );     // restore
                 
                    if (base.isIO) {
                       if (resources.maxIO == 0)   // take only first entry
                       {
                            resources.minIO = base.getAddress();
                            resources.maxIO = base.getAddress() + decoded_size;
                       }
                    } else {
                       if (resources.maxMEM==0)
                       {
                          resources.minMEM = base.getAddress();
                          resources.maxMEM = base.getAddress() + decoded_size;
                       }
                  }
              }
          }               
       }   // for now, other header types are ignored
     // TO DO: PCI-PCI bridges resources ??? 
   }
}

/*
bool 
IPCIDeviceImpl::init( IContext& context, NVPAIR params[] ) // INITSECTION
{
	return true;
}

void 
IPCIDeviceImpl::deinit( IContext& context )
{

}
*/

// virtual 
myos_result_t
IPCIDeviceImpl::getID( IO::OStream& out ) const {
    out.printf( "%x", ((u16) getVendorID() << 16) | (u16) getDeviceID() );
    return E_MYOS_SUCCESS;
}

// virtual 
myos_result_t
IPCIDeviceImpl::getCapabilities( IO::OStream& out ) const {
    return E_MYOS_E_NOTIMPL;
}

// virtual 
myos_result_t
IPCIDeviceImpl::hasDriver( IO::OStream& out ) const {
    out.put( driver!=0 ? '1' : '0' );
    return E_MYOS_SUCCESS;
}

bool
IPCIDeviceImpl::enable( MM::IVirtualMemory& mem, u32 flags )
{
   DPRINTK( "\nEnabling PCI device flags=%x ", flags );

   if ( !(flags&PCI_POWER_D0) || setPowerState( PCI_PM_CONTROL::D0 )) {
        // pci-pc.c : enableResources, enableIRQ
        // SimpleLock l; SimpleLock::PROTECTED region(l);
        {       
        PCI_COMMAND c = PCICONFIG_FIXED.command.read( info.handle );
        PCI_STATUS s = PCICONFIG_FIXED.status.read( info.handle );

        DPRINTK( "command=%x status=%x", (u32) c, (u32) s );

        /// Default: disable these, only enable when asked
        c.enableMEM = false;
        c.enableIO = false;

        // allocate memory. RTL8139 has res[0] == PIO, res[1] == MMIO, generally true ?
        if (flags&PCI_USE_MMIO) {
            if (resources.maxMEM > resources.minMEM) {
                info.MMIOstart = mem.mapMMIO( resources.minMEM, resources.maxMEM - resources.minMEM );
                c.enableMEM = true;
            } else {
                BUG( "No MMIO base address" );
                return false;
            }
        }

         /// Set IO base if present, could maintain mapping of used IO addresses
        if (flags & PCI_USE_PORTIO) {
            if (resources.maxIO > resources.minIO) {
                info.IObase = resources.minIO;
                info.IOsize = resources.maxIO - resources.minIO;
                c.enableIO = true;
            } else {
                BUG( "No IO base address" );
                return false;
            }
        }
                  
        // enable PCI busmaster if asked by driver
        c.MASTER = (flags & PCI_MASTER);
        c.FAST_BACK2BACK = s.PCI_STATUS_FAST_BACK;  // also depends on device-specific bits
        c.MWI = (flags & PCI_ENABLE_MWI);
        PCICONFIG_FIXED.command.write( info.handle, c ); // enable MMIO, disabled for now (fucks up RTL8139)
            
        DPRINTK( " new Command=%x status=%x", (u32) c,
            (u32) PCICONFIG_FIXED.status.read( info.handle ) );
           
        // in linux/arch/i386/kernel, the latency timer is checked (max_latency is a variable there, value = 255)
        u8 latTimer = PCICONFIG_FIXED.latency_timer.read( info.handle );
        if ((latTimer < 16) || (latTimer==0xff)) {
            DPRINTK( "\nFixing latency timer from %d to 64", latTimer );
            PCICONFIG_FIXED.latency_timer.write( info.handle, 64 );
        }           
        }
      return true;
   } else {
       DPRINTK( "\nSetPowerState failed!" );
   }
  return false;
}

bool    /// Powerstate D0 is often (always?) required to be able to write to IO/MMIO registers
IPCIDeviceImpl::setPowerState( PCI_PM_CONTROL::PCI_POWERSTATE ps )    
{
    ASSERTIONv( info.handle & 0x80000000, E_CRITICAL, info.handle ); 
   
   u8 offset = findCapability( PCI_CAP_ID_PM );
   if (offset)
    {
        // SimpleLock l; SimpleLock::PROTECTED region(l);
        PCI_PM_CONTROL control = PCICONFIG_VAR.PMControl.readOffset( info.handle, offset );         
        if ( control.powerstate != ps )
        {
            if (control.powerstate == PCI_PM_CONTROL::D3) {

                // devices loose configuration settings when switching to D3
                PCI_COMMAND c = PCICONFIG_FIXED.command.read( info.handle );
                PCICONFIG_FIXED.command.write( info.handle, c & ~3 );    // disable mem/IO

                // read the current configuration (linux reads only 5...)
                PCI_MEM_INFO base[6];
                for (s32 i=0; i<6; ++i) {
                    base[i] = PCICONFIG_FIXED.base_address0.readOffset( info.handle, 4*i );
                }
                PCI_MEM_INFO rom = PCICONFIG_FIXED.type0.ROM_address.read( info.handle );
                u8 latency = PCICONFIG_FIXED.latency_timer.read( info.handle );
                u8 cache_l = PCICONFIG_FIXED.cache_line_size.read( info.handle );

                // change power state
                control.powerstate = ps;                
                PCICONFIG_VAR.PMControl.writeOffset( info.handle, offset, control );
                
                // restore state
                for (s32 i2=0; i2<6; ++i2) {
                    PCICONFIG_FIXED.base_address0.writeOffset( info.handle, 4*i2, base[i2] );
                }
                PCICONFIG_FIXED.type0.ROM_address.write( info.handle, rom );
                if (info.assignedIRQ) {
                    PCICONFIG_FIXED.type0.interrupt_irq.write( info.handle, info.assignedIRQ );
                }
                PCICONFIG_FIXED.cache_line_size.write( info.handle, cache_l );
                PCICONFIG_FIXED.latency_timer.write( info.handle, latency );
                PCICONFIG_FIXED.command.write( info.handle, c );     // restore mem/IO
            } else {
                control.powerstate = ps;
                DPRINTK( "\nSetting PCI powerstate to %x", (u16) control );
                PCICONFIG_VAR.PMControl.writeOffset( info.handle, offset, control );
            }
        }
        return true;
    }
    return false;   // failure
}

u8
IPCIDeviceImpl::findCapability( PCI_CAPABILITY t ) const
{
    // SimpleLock l; SimpleLock::PROTECTED region(l);   
    PCI_STATUS status = PCICONFIG_FIXED.status.read( info.handle );
     
    /// Sometimes, this bit seems to get cleared! Check with PCI documentation and/or device docs
    if (status.PCI_STATUS_CAP_LIST) {
        u8 offset;
        switch ( header_type.type )
        {
        case PCI_HEADER_TYPE::TYPE0 :
        case PCI_HEADER_TYPE::PCI_PCI_BRIDGE :
            offset = PCICONFIG_FIXED.type0.first_capability.read( info.handle ); break;      
        case PCI_HEADER_TYPE::CARDBUS :
            offset = PCICONFIG_FIXED.cardbus.first_capability.read( info.handle ); break;
           
        default:
            BUG( "Unknown PCI header_type read" );
            offset=0;   
        }
      
        for (u32 n=0; ((n<48) && (offset>=0x40)); ++n)
        {
            PCI_CAPABILITY cap = PCICONFIG_VAR.capabilities.readOffset( info.handle, offset );
            if (cap==t) return offset;
            else if (cap==PCI_END_OF_LIST) break;
            offset = PCICONFIG_VAR.capabilities.readOffset( info.handle, offset+1 ); // next
        }
   } else DPRINTK( "PCI device does not support capabilities" );

   DPRINTK( "\nCapability not found: %x for device %x", (u32) t, (u32) info.handle );
   return 0;   // not found
}

void
IPCIDeviceImpl::releaseResources( MM::IVirtualMemory& mem )
{    
    driver = 0;
    if (info.MMIOstart) {
        DPRINTK( "\nReleasing MMIO resources..." );
        mem.unmapMMIO( info.MMIOstart, resources.maxMEM - resources.minMEM );
        info.MMIOstart = 0;
    }

    /// linux/drivers/pci/pci.c: if bus mastering is enabled, disable it
    {
      // SimpleLock l; SimpleLock::PROTECTED region(l);
      PCI_COMMAND command = PCICONFIG_FIXED.command.read( info.handle );
      if (command.MASTER) {
         command.MASTER = false;
         PCICONFIG_FIXED.command.write( info.handle, command );
      }
    }
}

}}}  // namespaces
