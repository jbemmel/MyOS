#include "FATDirectory.h"
#include "FATDirEntry.h"
#include "FAT12.h"
#include "FATFileFragment.h"

#include "FS/FAT/FATComponent.h"
#include "MM/new.h"

// debug
#include "FATFileInfo.h"

namespace MyOS { 
  
namespace Context {

template<> inline void* TreapNode<MyOS::FS::FAT::FATFileNode>::operator new( size_t )
{
   return MyOS::FS::FAT::allocate( sizeof(TreapNode<FS::FAT::FATFileNode>) );
}

template<> inline void TreapNode<MyOS::FS::FAT::FATFileNode>::operator delete( void* m )
{
   MyOS::FS::FAT::deallocate( m, sizeof(TreapNode<FS::FAT::FATFileNode>) );
}

#include "Context/Treap.hpp"

}
  
namespace FS { namespace FAT {

FATDirectory::FATDirectory()
: IDirectory( FATComponent::getInstance(), VERSION(1,0) )
, name("FAT12"), parent(0), initialized(false), files(FATFileNode( myos_name_t("<not found>") ))
{
   
}

FATDirectory::FATDirectory( FATFileInfo& info, FATDirectory& p )
: IDirectory( FATComponent::getInstance(), VERSION(1,0) )
, name(info.getName()), parent(&p), initialized(false), files(FATFileNode( myos_name_t("<not found>") ))
{
   
}


FATDirectory::~FATDirectory()
{
   // this destroys files too
   // DPRINTK( "\n~FATDirectory()" );
}

void
FATDirectory::init( FATDirEntry* entries, size_t nEntries, const FAT12& fat )
{ 
   // Must make sure of this, since memory allocations cannot be done in IRQ
   // NOT_IN_IRQ; Now using NOTHROW memory
   for (u32 i=0; i<nEntries && !entries[i].isLast(); ++i) {
     
      // filters '.' and '..' too!
      if (!entries[i].shouldSkip()) {
         FATFileNode node( entries[i], fat );
         files.insert( node );  
      }
   }
      
   // DPRINTK( "\nDIR init done" );
   initialized = true;   
}

// virtual
myos_name_t
FATDirectory::findNode( const myos_name_t& nodename, VERSION v, IInterface*& result ) const
{
   FATFileNode search( nodename );   // XX version ignored
   search = files.findFirst( search );
   if (search) {
      // TODO: if it was a subdir, continue searching
      result = (IInterface*) search.getInfo();     // XX not nice
      return nodename.substring( nodename.length );        // rest, length=0
   }
   return nodename;   // not found
}


// virtual 
myos_result_t 
FATDirectory::list( myos_name_t start, u32 recursiondepth, IO::OStream& out ) const
{
   // This should normally not be needed...
   if (!isInitialized()) return E_MYOS_ERROR;

   // assuming start=="", ignoring depth for now...
   out.printf( "\n<root>" );
   FATFileNode::toXMLparameters params = { out, recursiondepth };
   int count = files.foreach( &FATFileNode::toXML, &params ); 
   out.printf( "\n<total filecount=\"%d\"/>\n</root>", count );
   return E_MYOS_SUCCESS;
}
  
}}}   // namespace
