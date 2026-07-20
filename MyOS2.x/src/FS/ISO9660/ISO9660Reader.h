#ifndef ISO9660READER_H
#define ISO9660READER_H

#include "Devices/IBlockDevice.h"
#include "../IMountedFS.h"
#include "FS/ISO9660/IISO9660FS.h"

#include "Context/IDirectory.h"     // seems a bit too large for my initial purp
#include "ISO9660Directory.h"

namespace MyOS { namespace FS { namespace ISO9660 {

using namespace Devices;
using Context::IDirectory;

class ISO9660FileInfo;

class ISO9660Reader : public IMountedFS, public Devices::IOCompletionHandler
{
   ISO9660FileInfo& processDirs( const myos_name_t& filepath ) const
      throw (FileNotFoundException);
public:
   /**
    * Instantiate one of these for each of the devices that use ISO9660
    */
   void* operator new( size_t );
   void operator delete( void* );    
    
   ISO9660Reader( IBlockDevice& device );   
   virtual ~ISO9660Reader() { if (device) device->close(); } 

   // IMountedFS
   virtual IOpenFile& openFilePaged( const myos_name_t & filepath )
      throw (FileNotFoundException, FSException);

   virtual IOpenFileAsync& openFileAsync( const myos_name_t & filepath ) const
      throw (FileNotFoundException, FSException);

   virtual void deleteFile( const myos_name_t & filepath ) 
      throw (FSException);

   virtual void renameFile( const myos_name_t & filepath, const myos_name_t& newname ) 
      throw (FSException);

   virtual void unmount() throw (FSException);

   // (will be in) XML
   virtual  myos_result_t readFile( 
      const myos_name_t& filepath,IO::OStream& out   
   ) const;

   virtual  myos_result_t enumerateFiles(
      IO::OStream& out   
   ) const;

   // These could be in a generic FS interface
   const IDirectory& getRoot() const;  // or IContext ??

   IBlockDevice& getDevice() const { return *device; }

private:
   // IOCompletionHandler
   virtual u32 onReadDone( IORequest& rq, const buffer& data );
   virtual void onError( u32 lbnStart, size_t count );

   IBlockDevice* device;
   
   ISO9660Directory root;
   
   // Relevant data from the boot sector, copied upon startup
   
};
  
}}}   // namespace

#endif
