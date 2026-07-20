#include "ISO9660FileNode.h"
#include "ISO9660FileInfo.h"
#include "PrimaryVolumeDesc.h"

#include "FS/ISO9660/ISO9660Component.h"
#include "MM/new.h"

namespace MyOS { namespace FS { namespace ISO9660 {

inline void* ISO9660FileInfo::operator new( size_t ) {
   return MyOS::FS::ISO9660::allocateNoThrow( sizeof(ISO9660FileInfo) );
}

inline void ISO9660FileInfo::operator delete( void* p ) {
   // DPRINTK( "\nISO9660FileInfo::delete" );
   MyOS::FS::ISO9660::deallocate( p, sizeof(ISO9660FileInfo) );
}


ISO9660FileNode::ISO9660FileNode( const FileInfo& f )
: name(mszName,f.getName(mszName) )
{
   // Sanity check, size==0 for directories
   if (f.filesize > (700 * _1MB) ) {
      DPRINTK( "\nBogus size=%d, skipping file %n", f.filesize, &name );
      return;
   }
   // e.print( COUT );    // debug

   info = new ISO9660FileInfo( *this, f );
}

// This constructor is used by the treap
ISO9660FileNode::ISO9660FileNode( const ISO9660FileNode& n ) 
: name(mszName,0), info(n.info)
{
   name.copy(n.name);
      
   // take over(!) the info node, 'n' will be destroyed soon!
   if (info) info->setNode( *this );             
}


ISO9660FileNode::~ISO9660FileNode()
{   
   // only upon removal from the tree!
   if (info && info->isOwner(*this)) {
      // DPRINTK( "\n#4f#deleting info %x...", info );
      delete info;
      info = 0; 
   }
}

s32
ISO9660FileNode::toXML( void* p )
{
   ((toXMLparameters*)p)->out.printf( "\n<file name=\"%n\" size=\"%u\"/>", 
      &name, info->getFileSize() );
   return 1;
}

}}}   // namespace
