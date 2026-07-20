#ifndef BIOSComponent_H
#define BIOSComponent_H

#include "interfaces.h"
#include "Core/IComponent.h"
#include "BIOS/IBIOSImpl.h"
#include "MM/Virtual/ByteAllocator.h"


namespace MyOS { namespace BIOS {

/****
 * Dependencies
 ***/
using MyOS::Core::IComponent;
using MM::IVirtualMemory;
using MM::IPhysicalMemory;
using InterruptHandling::IInterruptHandling;



class BIOSComponent : public IComponent {
public:  // easier
IVirtualMemory* iVirtualMemory;
IPhysicalMemory* iPhysicalMemory;
IInterruptHandling* iInterruptHandling;

public:
	inline static const char* const ID() { return "ea03802f-aa66-461c-8ff9-90fb7d046e5e"; }

    BIOSComponent( MyOS::Core::IContainer& container ) throw() INITSECTION;
    virtual myos_result_t initialize( Context::IContext& context, NVPAIR params[] ) INITSECTION;
    virtual myos_result_t deinitialize( Context::IContext& context );

	virtual myos_result_t queryInterface(
      IComponent& requestor,
      const uuid_t& uuid,
      IInterface*& result
    );

private:
    static BIOSComponent* instance;
public: 
	inline static BIOSComponent& getInstance() { return *instance; }

private:    

   friend class IBIOSImpl;	// access interfaces
   IBIOSImpl impl1;

	public: MM::ByteAllocator allocator;


};


// within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate( size_t s ) throw (MM::OutOfMemoryException) {
   return BIOSComponent::getInstance().allocator.allocate( s );
}

inline void* allocateNoThrow( size_t s ) throw () {
   return BIOSComponent::getInstance().allocator.allocateNoThrow( s );
}

inline void deallocate( void* p, size_t s ) {
   BIOSComponent::getInstance().allocator.deallocate( p, s );
}

}} // namespaces



#ifndef NEW_DECLARED
#define NEW_DECLARED

// Declared in global namespace
inline void* operator new( size_t size ) throw (MyOS::MM::OutOfMemoryException) {
	return MyOS::BIOS::allocate(size);  
}

inline void* operator new[]( size_t size ) throw (MyOS::MM::OutOfMemoryException) {
	return MyOS::BIOS::allocate(size);  
}

inline void operator delete( void* m, size_t s ) {
	MyOS::BIOS::deallocate(m,s);
}

#endif	// NEW_DECLARED



#endif
