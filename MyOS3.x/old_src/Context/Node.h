/***************************************************************************
                          Nodeimpl.h  -  description
                             -------------------
    begin                : Sun Sep 2 2001
    copyright            : (C) 2001 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl
 ***************************************************************************/
#ifndef NODE_H
#define NODE_H

// #include "IO/IOUtil.h"     // strcmpa, optional
#include "Version.h"
#include "Name.h"       // myos_name_t

// debug
#include "debug.h"

namespace MyOS {

class buffer;

namespace Core { class IInterface; }
using Core::IInterface;

namespace IO {
   class OStream;
   class IOUtil;
}

namespace Context {

// Cannot use static objects for these, they don't get initialized!
#define SELF   myos_name_t(".",1)
#define PARENT myos_name_t("..",2)

using IO::IOUtil;
class IDirectoryCallback;

/**
 * Implementation class for all directory nodes. A basic directory node is very simple, it
 * has a myos_name_t and a pointer to a single interface it supports
 *
 * @author Jeroen van Bemmel
 */
class NodeIMPL /* : public I_Node */ {
   typedef const char* myos_call_t;

   // prevent dynamic allocation
   inline void operator delete( void* ) {}

public:
   inline NodeIMPL( const myos_name_t& n, VERSION v, IInterface* i=0 )
	    : name(n), version(v), myInterface(i) {}

   /// Like the above, but considers only l characters of name
   inline NodeIMPL( const myos_name_t& n, size_t l, VERSION v, IInterface* i=0 )
	    : name(n), version(v), myInterface(i) {}


	// required for treap: default constructor, operator <
	inline NodeIMPL() : name(), version(VERSION::ANY()), myInterface(0) {}

	inline NodeIMPL( const NodeIMPL& n )
        : name(n.name), version(n.version), myInterface( n.myInterface ) {}

	/// Destructor, reset values
	inline ~NodeIMPL() {
        name.name=0; name.length=0; version=0; myInterface=0; 
    }

	/// Compare two nodes by name, uses strcmpa for proper number sorting
	inline bool operator <( const NodeIMPL& other ) const
	{
      // DPRINTK( "\ncmp %n with %n", &name, &other.name );
     
      // full match
      // s32 s = IOUtil::strncmpa( name.name, other.name.name, name.length <? other.name.length );
      s32 s = strncmp( name.name, other.name.name, min( name.length, other.name.length) );

      if (s==0) s = name.length - other.name.length;

      // Make sure that higher versions are found first when searching!
      return (s<0) || (s==0 && version>other.version);
	}

	inline NodeIMPL& operator =( const NodeIMPL& other )
	{
        name = other.name;
        version = other.version;
        myInterface = other.myInterface;
        return *this;
	}

	IInterface& getInterface() const { return *myInterface; }
	myos_name_t getName() const      { return name; }
    VERSION getVersion() const       { return version; }

	/// Reuse the node (same myos_name_t, different interface)
	inline void reuse( IInterface& forInterface, VERSION v ) const {
		version = v;
		myInterface = &forInterface;
	}

    /// Check if a node is valid
    inline operator bool() const {
        return (myInterface != 0);
    }

    struct toXMLparameters {
        IO::OStream& out;
        int levels;
    };
    s32 toXML( void *params );

   struct callbackParameters {
      IDirectoryCallback &callback;
      void *context;
      u32 flags;
   };
   s32 callback( void *params );

private:
   myos_name_t name;
   mutable VERSION version;            // reused
   mutable IInterface* myInterface;		// can be reused
};

}} // namespace

#endif
