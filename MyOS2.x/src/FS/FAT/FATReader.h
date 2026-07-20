#ifndef FATREADER_H
#define FATREADER_H

#include "Devices/IBlockDevice.h"
#include "FS/FAT/IFATFS.h"
#include "FATDirectory.h"

namespace MyOS { namespace FS { namespace FAT {

using namespace Devices;

// TODO: Define a generic FS interface
class FATReader : public IMountedFS, public Devices::IOCompletionHandler
{
   FATFileInfo& processDirs( const myos_name_t& filepath ) const
      throw (FileNotFoundException);
public:
   /**
    * Instantiate one of these for each of the devices that use FAT
    */
   void* operator new( size_t );
   void operator delete( void* );    
    
   FATReader( IBlockDevice& device );   
   virtual ~FATReader() { if (device) device->close(); } 

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
   buffer fatbuffer; // could keep this for writing...
   FATDirectory root;
   
   // Relevant data from the boot sector, copied upon startup
   u16 rootEntries;
   
   // TODO: bitmap for free sectors
};
  
}}}   // namespace

#endif
