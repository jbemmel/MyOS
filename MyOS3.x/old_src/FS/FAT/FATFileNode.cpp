#include "FATFileNode.h"
#include "FATDirEntry.h"
#include "FATFileFragment.h"
#include "FATFileInfo.h"
#include "FAT12.h"

#include "FS/FAT/FATComponent.h"
#include "MM/new.h"

namespace MyOS { namespace FS { namespace FAT {

inline void* FATFileInfo::operator new( size_t ) {
	return MyOS::FS::FAT::allocateNoThrow( sizeof(FATFileInfo) );
}

inline void FATFileInfo::operator delete( void* p ) {
   // DPRINTK( "\nFATFileInfo::delete" );
   MyOS::FS::FAT::deallocate( p, sizeof(FATFileInfo) );
}


FATFileNode::FATFileNode( const FATDirEntry& e, const FAT12& fat )
: name(mszName,e.getName(mszName) )
{
   // Sanity check, size==0 for directories
   if (e.getSize() > (2880 * 512) ) {
      DPRINTK( "\nBogus size=%d, skipping file %n", e.getSize(), &name );
      return;
   }
   // e.print( COUT );    // debug

   info = new FATFileInfo( *this, e, fat );
}

// This constructor is used by the treap
FATFileNode::FATFileNode( const FATFileNode& n ) 
: name(mszName,0), info(n.info)
{
   name.copy(n.name);
      
   // take over(!) the info node, 'n' will be destroyed soon!
   if (info) info->setNode( *this );             
}


FATFileNode::~FATFileNode()
{   
   // only upon removal from the tree!
   if (info && info->isOwner(*this)) {
      // DPRINTK( "\n#4f#deleting info %x...", info );
      delete info;
      info = 0; 
   }
}

s32
FATFileNode::toXML( void* p )
{
   ((toXMLparameters*)p)->out.printf( "\n<file name=\"%n\" size=\"%u\"/>", 
      &name, info->getFileSize() );
   return 1;
}

}}}   // namespace
