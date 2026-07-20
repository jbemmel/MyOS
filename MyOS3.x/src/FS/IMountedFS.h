#ifndef IMOUNTEDFS_H
#define IMOUNTEDFS_H

#include "IOpenFile.h"

namespace MyOS
{
namespace Devices
{
class IBlockDevice;
}

namespace FS
{

// TODO: Should be derived from IInterface?
class IMountedFS
{
public:

    virtual IOpenFile& openFilePaged(const myos_name_t & filepath) throw(
            FileNotFoundException, FSException) = 0;

    /// Open file for non-blocking operations
    /**
     * Note that this operation itself is blocking (it needs to wait for IO)
     */
    virtual IOpenFileAsync
            & openFileAsync(const myos_name_t & filepath) const throw(
                    FileNotFoundException, FSException) = 0;

    /// More efficient than opening file first!
    virtual void deleteFile(const myos_name_t & filepath) throw(FSException) = 0;

    /// Renames target file
    /**
     * @param newname: New last part of the filename, cannot contain '/'
     */
    virtual void renameFile(const myos_name_t & filepath,
            const myos_name_t& newname) throw(FSException) = 0;

    virtual myos_result_t
            readFile(const myos_name_t& filepath, IO::OStream& out) const = 0;

    virtual myos_result_t enumerateFiles(IO::OStream& out) const = 0;

    virtual void unmount() throw(FSException) = 0;

};

} // namespace FS
} // namespace MyOS

#endif
