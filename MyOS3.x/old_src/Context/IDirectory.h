/***************************************************************************
                          Directory.h  -  description
                             -------------------
    begin                : Sun Sep 2 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl

    I first created the InterfaceRepository. But soon I discovered that that was not enough:
    If you have multiple instances (eg devices) implementing the same interface, how do
    you select the right one?

    So there has to be a hierarchical naming structure. The InterfaceRepository is part of
    that (and of course accessible through it)

    @todo Allow wildcard searches -> requires >1 result array
    @todo Add flag 'ONLY_LOCAL' and possibility to register links to remote sites
    @todo Add 'createValueEntry' for simple <name,type,value> items (+IValueEntry interface)
    
    @todo Split into read-only operations and read-write operations (derived from RO)
 ***************************************************************************/

#ifndef IDIRECTORY_H
#define IDIRECTORY_H

#include "Core/IInterface.h"
#include "Version.h"

namespace MyOS {

struct myos_name_t;

namespace IO {
   class OStream;
}

/// Contains all Directory related classes
namespace Context {

typedef const char* myos_path_t;

using Core::IInterface;

/**
 * Callback interface for extending, eg generating HTML for the directory
 */
class IDirectoryCallback
{
public:
   // may return false to stop iterating
   virtual bool onIterate( const myos_name_t &name, IInterface& i, void* context ) = 0;
};

/**
 * Interface to filesystem-like functionality: Store nodes, retrieve nodes, create subdirectories
 *
 * This interface differs a bit from other INTERFACE classes since there may be many implementations
 * (each subdirectory implements IDirectory). The Directory component registers the root directory under
 * the interface UUID for IDirectory, from there you can navigate further
 * 
 * @todo add 'getRoot'? can be done using findNode("/") though
 */
INTERFACE( IDirectory, "94625155-52fc-48ad-88a8-bf40d3dfa396" )   


/// Flags for addNode() and createSubdirectory()
enum E_CREATEFLAGS {
   E_NONE                     = 0,
   E_REUSE_IF_EXISTS         = 0x1, //< used for createDirectory, addNode
   E_ALLOW_MULTIPLE_VERSIONS = 0x2, //< used for addNode
};

/// Flags for #onAllNodes()
enum E_ACCEPTFLAGS {
   A_NONE = 0,
   A_STOP_AT_FAILURE = 0x1,   //< Stop at first node that returns negative result on accept()
   A_RECURSIVE       = 0x2,   //< Perform same call on subnodes (recursively), depth-first
};

/// The character used for separating directory names in paths
#define SEPARATOR '/'

/// Returns the name of this directory
virtual const myos_name_t& getName() const = 0;

/// Adds a node to this directory
/*
 * @param name    : The name under which to register this node
 * @param i       : The interface to register under this name (version will be read from it)
 * @param flags   : <optional> flags : REUSE_IF_EXISTS, ALLOW_MULTIPLE_VERSIONS
 * @return  E_MYOS_SUCCESS if succeeded
 * @return  E_MYOS_E_INVALIDARG if version==VERSION::ANY()
 * @return  E_MYOS_REUSED  if <name> exists but 'REUSE_IF_EXISTS' was set
 * @return  E_MYOS_EEXIST  if <name> exists (possibly under a different version)
 */
virtual myos_result_t addNode( const myos_name_t& name, IInterface& i, u32 flags=E_NONE ) = 0;

/// Removes a node from this directory
/*
 * @param name    : The name of the node, null terminated
 * @param version : <optional> The version to remove, first one found will be removed if none given
 *
 * @return pointer to node's IInterface if success, 0 if not found
 * @note This can be used to free resources associated with the IInterface object
 */
virtual IInterface* removeNode( const myos_name_t& name, VERSION version = VERSION::ANY() ) = 0;

/// Creates a subdirectory
/**
 * @param name    : The name of the new subdirectory (should not contain SEPARATOR chars)
 * @param flags   : Creation flags, see CREATEFLAGS  (default: NONE)
 * @return a valid pointer, or <code> null </code> if failed (e.g. exists and no special flag)
 *
 * @note The caller is responsible for deleting the directory when finished
 */
virtual IDirectory* createSubdirectory( const myos_name_t& name, u32 flags=E_NONE ) = 0;

/// Removes a subdirectory
/**
 * The subdirectory is no longer child of its parent, but resources are not freed yet
 * In this way, it can be moved to another location
 *
 * @param dir     : The directory to remove (not deleted)
 * @return success
 */
virtual bool removeSubdirectory( IDirectory &dir ) = 0;

/// Creates a link to another directory
/**
 * @todo Also allow nodes ? requires return of INode handle somewhere else


 * @note Use in conjunction with TFTP impl for link to root dir
 */
virtual myos_result_t createLink( const myos_name_t& name, IDirectory &toLink, u32 flags ) = 0;

/// Frees resources used by this directory
/**
 * @note removes it from the parent directory (if any & not already removed)
 * @return true iff success
 */
virtual bool destroy() = 0;

/// Finds an interface instance in this directory
/**
 * @param path    :  Path (relative or absolute) to the node (may include subdirectories)
 * @param version :  <optional> implementation version of the interface, use VERSION::ANY() for dont care
 * @param result  : [out] result of search
 * @return part of path that was not resolved
 * 
 * @todo rename to findInterface or findInstance?
 */
virtual myos_name_t 
findNode( const myos_name_t& path, VERSION version, IInterface*& result ) const = 0;

/// Lists the contents of this directory or a subdir (and optionally childnodes) to outputstream
/**
 * @param start            : Path to start the listing
 * @param recursiondepth   : Max. levels to list
 * @param ostream          : Outputstream to write listing to
 * @return #bytes written if success
 * <br> E_MYOS_E_INVALIDARG if <start> is not a valid path
 * @todo more errorcodes
 * @todo move to separate interface?
 */
virtual myos_result_t list( myos_name_t start, u32 recursiondepth, IO::OStream& ostream ) const = 0;

/// Will call callback for all subnodes, optionally recursive, returns #nodes that were visited
/**
 * Iterates recursively over all nodes in this IDirectory, and [optionally] sub directories
 * 
 * @param callback - caller-provided callback interface
 * @param context  - opaque parameter passed to callback
 * @param flags    - flags, see IDirectory::E_ACCEPTFLAGS
 */
virtual u32 onAllNodes( IDirectoryCallback& callback, void* context, u32 flags ) const = 0;
};

}} // namespace

#endif
