#ifndef RepositoryComponent_H
#define RepositoryComponent_H

#include "Core/IComponent.h"

#include "Context/IContextImpl.h"

#include "MM/Virtual/ByteAllocator.h"


namespace MyOS { namespace Context {

using MyOS::Core::IComponent;

class RepositoryComponent : public IComponent {
	static RepositoryComponent* instance;

	friend class IContextImpl;	// access interfaces
	IContextImpl impl1;

public:  // easier
	inline static const char* const ID() { return "94625155-52fc-48ad-88a8-bf40d3dfa398"; }
	
	IVirtualMemory* iVirtualMemory;
	MM::Allocator allocator;
	

public:
    RepositoryComponent( MyOS::Core::IContainer& container ) throw() INITSECTION;
    virtual myos_result_t initialize( IContext& context, NVPAIR params[] ) INITSECTION;
    virtual myos_result_t deinitialize( IContext& context );

	virtual myos_result_t queryInterface(
      IComponent& requestor,
      const uuid_t& uuid,
      IInterface*& result
    );

	inline static RepositoryComponent& getInstance() { return *instance; }
	inline IContext& getContext() { return impl1; }
};


// within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate( size_t s ) throw (MM::OutOfMemoryException) {
   return RepositoryComponent::getInstance().allocator.allocateAutoSize( s );
}

inline void* allocateNoThrow( size_t s ) throw () {
   return RepositoryComponent::getInstance().allocator.allocateAutoSizeNoThrow( s );
}

inline void deallocate( void* p ) {
   RepositoryComponent::getInstance().allocator.freeAutoSize( p );
}

}} // namespaces



#ifndef NEW_DECLARED
#define NEW_DECLARED

// Declared in global namespace
inline void* operator new( size_t size ) throw (MyOS::MM::OutOfMemoryException) {
	return MyOS::Context::allocate(size);  
}

inline void* operator new[]( size_t size ) throw (MyOS::MM::OutOfMemoryException) {
	return MyOS::Context::allocate(size);  
}

inline void operator delete( void* m ) {
	MyOS::Context::deallocate(m);
}

#endif	// NEW_DECLARED



#endif
