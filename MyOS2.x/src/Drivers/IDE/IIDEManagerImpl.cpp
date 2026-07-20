#include "myosconfig.h"
#ifdef CONFIG_IDE

#include "IIDEManagerImpl.h"
#include "Init/memlayout.h"   // bioshddinfo
#include "debug.h"

namespace MyOS { namespace Drivers { namespace IDE {

IIDEManagerImpl::IIDEManagerImpl( MyOS::Core::IComponent& c )
: IIDEManager( c, VERSION(1,0) )
, primary( IDEController::PRIMARY ), secondary( IDEController::SECONDARY )
{

}

bool 
IIDEManagerImpl::init( IContext& context, NVPAIR params[] ) /* INITSECTION */
{
   // read the Disk tables constructed by bootphase2.asm at 0x7800
   // see http://web.inter.nl.net/hcc/J.Steunebrink/bioslim.htm

   struct BIOSHDDINFO {
      u16 size;
      u16 flags;
      u32 c, h, s;
      u64 totalSectors;
      u16 bytesPerSector;
   } *info = (BIOSHDDINFO*) bioshddinfo;
   DPRINTK( "\nHDD INFO[0].size = %u C=%u H=%u S=%u", info->size, info->c, info->h, info->s ); 
   info = (BIOSHDDINFO*) ((u8*) info) + info->size;
   DPRINTK( "\nHDD INFO[1].size = %u C=%u H=%u S=%u", info->size, info->c, info->h, info->s ); 
   info = (BIOSHDDINFO*) ((u8*) info) + info->size;
   DPRINTK( "\nHDD INFO[2].size = %u C=%u H=%u S=%u", info->size, info->c, info->h, info->s ); 
   info = (BIOSHDDINFO*) ((u8*) info) + info->size;
   DPRINTK( "\nHDD INFO[3].size = %u C=%u H=%u S=%u", info->size, info->c, info->h, info->s ); 
   info = (BIOSHDDINFO*) ((u8*) info) + info->size;
 
   // If primary fails, secondary will not even be tried!
   return primary.init() && secondary.init();
}

void 
IIDEManagerImpl::deinit( IContext& context )
{

}

// virtual 
bool 
IIDEManagerImpl::getDevice( u32 id, IIDEDevice*& dev ) const
{
   DPRINTK( "\ngetDevice IDE id=%X", id ); 
 
   IIDEDevice* d = 0;   // keep gcc happy
   switch (id)
   {
   case 0: d = primary.getMaster(); break;
   case 1: d = primary.getSlave(); break;
   case 2: d = secondary.getMaster(); break;
   case 3: d = secondary.getSlave(); break;    
   }
   if (d) {
      dev = d;
      return true;         
   }
   return false;
}

// virtual 
myos_result_t 
IIDEManagerImpl::listDevices( IO::OStream& out ) const {
   return E_MYOS_E_NOTIMPL;
}

}
}}  // namespaces

#endif
