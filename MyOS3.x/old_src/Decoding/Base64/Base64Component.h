#ifndef Base64Component_H
#define Base64Component_H

#include "Core/IComponent.h"
#include "Decoding/Base64/IBase64Impl.h"
#include "MM/Virtual/ByteAllocator.h"


namespace MyOS { namespace Decoding { namespace Base64 {

/****
 * Dependencies
 ***/
using MyOS::Core::IComponent;
using Devices::Display::IDisplay;
using MM::IVirtualMemory;



class Base64Component : public IComponent {
public:  // easier
IDisplay* iDisplay;
IVirtualMemory* iVirtualMemory;

public:
	inline static const char* const ID() { return "400368fd-5c1e-4cb0-8a7e-04dce188b23a"; }

    Base64Component( MyOS::Core::IContainer& container ) throw() INITSECTION;
    virtual myos_result_t initialize( Context::IContext& context, NVPAIR params[] ) INITSECTION;
    virtual myos_result_t deinitialize( Context::IContext& context );

	virtual myos_result_t queryInterface(
      IComponent& requestor,
      const uuid_t& uuid,
      IInterface*& result
    );

private:
    static Base64Component* instance;
public: 
	inline static Base64Component& getInstance() { return *instance; }

private:    

   friend class IBase64Impl;	// access interfaces
   IBase64Impl impl1;

	public: MM::ByteAllocator allocator;


};


// within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate( size_t s ) throw (MM::OutOfMemoryException) {
   return Base64Component::getInstance().allocator.allocate( s );
}

inline void* allocateNoThrow( size_t s ) throw () {
   return Base64Component::getInstance().allocator.allocateNoThrow( s );
}

inline void deallocate( void* p, size_t s ) {
   Base64Component::getInstance().allocator.deallocate( p, s );
}
}
}} // namespaces



#ifndef NEW_DECLARED
#define NEW_DECLARED

// Declared in global namespace
inline void* operator new( size_t size ) throw (MyOS::MM::OutOfMemoryException) {
	return MyOS::Decoding::Base64::allocate(size);  
}

inline void* operator new[]( size_t size ) throw (MyOS::MM::OutOfMemoryException) {
	return MyOS::Decoding::Base64::allocate(size);  
}

inline void operator delete( void* m, size_t s ) {
	MyOS::Decoding::Base64::deallocate(m,s);
}

#endif	// NEW_DECLARED



#endif
