#ifndef TimerComponent_H
#define TimerComponent_H

#include "IComponent.h"

#include "ITimingImpl.h"
#include "MM/ByteAllocator.h"

namespace MyOS {
namespace Timer {

using MyOS::Core::IComponent;

class TimerComponent : public IComponent {
public:
	static inline const char* const ID() { return "a000f904-ce87-4c4a-a055-804df647ec5b"; }

	IVirtualMemory* iVirtualMemory;
	IInterruptHandling* iInterruptHandling;

	TimerComponent(MyOS::Core::IContainer& container) throw() INITSECTION;
	virtual myos_result_t initialize(Context::IContext& context,
			NVPAIR params[]) INITSECTION;
	virtual myos_result_t deinitialize(Context::IContext& context);

	virtual myos_result_t queryInterface(IComponent& requestor,
			const uuid_t& uuid, IInterface*& result);

private:
	static TimerComponent* instance;

public:
	inline static TimerComponent& getInstance() throw() {
		return *instance;
	}

	inline static ITimerFacilityImpl& getTFImpl() { return instance->impl1; }
	
private:

	friend class ITimingImpl; // access interfaces
	friend class C8254;
	ITimerFacilityImpl impl1;

public:
	MM::ByteAllocator allocator;

};

// within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate(size_t s) throw(MM::OutOfMemoryException) {
	return TimerComponent::getInstance().allocator.allocate(s );
}

inline void* allocateNoThrow(size_t s) throw() {
	return TimerComponent::getInstance().allocator.allocateNoThrow(s );
}

inline void deallocate(void* p, size_t s) {
	// DPRINTK( "deallocate(%x) s=%u", p, s );
    TimerComponent::getInstance().allocator.deallocate(p, s );
}

}
} // namespaces


#ifndef NEW_DECLARED
#define NEW_DECLARED

// Declared in global namespace
inline void* operator new(size_t size) throw(
		MyOS::MM::OutOfMemoryException) {
	return MyOS::Timer::allocate(size);
}

inline void* operator new[](size_t size) throw(
		MyOS::MM::OutOfMemoryException) {
	return MyOS::Timer::allocate(size);
}

inline void operator delete(void* m, size_t s) {
	MyOS::Timer::deallocate(m, s);
}

#endif	// NEW_DECLARED
#endif
