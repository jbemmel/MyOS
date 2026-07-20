#include "ISO9660Directory.h"

#include "FS/ISO9660/ISO9660Component.h"
#include "MM/new.h"

// debug
#include "ISO9660FileInfo.h"

namespace MyOS { namespace FS { namespace ISO9660 {

template<> inline void* TreapNode<ISO9660FileNode>::operator new( size_t )
{
   return MyOS::FS::ISO9660::allocateNoThrow( sizeof(TreapNode<FS::ISO9660::ISO9660FileNode>) );
}

template<> inline void TreapNode<ISO9660FileNode>::operator delete( void* m )
{
   MyOS::FS::ISO9660::deallocate( m, sizeof(TreapNode<FS::ISO9660::ISO9660FileNode>) );
}

#include "Context/Treap.hpp"

ISO9660Directory::ISO9660Directory()
: IDirectory( ISO9660Component::getInstance(), VERSION(1,0) )
, name("ISO9660"), parent(0), initialized(false), files(ISO9660FileNode( myos_name_t("<not found>") ))
{
   
}

ISO9660Directory::ISO9660Directory( ISO9660FileInfo& info, ISO9660Directory& p )
: IDirectory( ISO9660Component::getInstance(), VERSION(1,0) )
, name(info.getName()), parent(&p), initialized(false), files(ISO9660FileNode( myos_name_t("<not found>") ))
{
   
}


ISO9660Directory::~ISO9660Directory()
{
   // this destroys files too
   // DPRINTK( "\n~ISO9660Directory()" );
}

void
ISO9660Directory::init( FileInfo* entries )
{ 
   // Must make sure of this, since memory allocations cannot be done in IRQ
   // NOT_IN_IRQ; Now using NOTHROW memory

   // On ISO9660, directories always have minimal 2 entries: '.' and '..'
   // Skip those...
   entries += 2; // adds 2 * sizeof(FileInfo);
   
   while (entries->infosize)
   {
      ISO9660FileNode node( *entries );
      files.insert( node );  
      
      entries = (FileInfo*) (((u8*) entries) + entries->infosize);      
   }
      
   DPRINTK( "\nDIR init done" );
   initialized = true;   
}

// virtual
myos_name_t
ISO9660Directory::findNode( const myos_name_t& nodename, VERSION v, IInterface*& result ) const
{
   ISO9660FileNode search( nodename );   // XX version ignored
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
ISO9660Directory::list( myos_name_t start, u32 recursiondepth, IO::OStream& out ) const
{
   // This should normally not be needed...
   if (!isInitialized()) return E_MYOS_ERROR;

   // assuming start=="", ignoring depth for now...
   out.printf( "\n<root>" );
   ISO9660FileNode::toXMLparameters params = { out, recursiondepth };
   int count = files.foreach( &ISO9660FileNode::toXML, &params ); 
   out.printf( "\n<total files=\"%d\"/>\n</root>", count );
   return E_MYOS_SUCCESS;
}
  
}}}   // namespace
