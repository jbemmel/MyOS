#ifndef MTComponent_H
#define MTComponent_H

#include "IComponent.h"

#include "IMultiThreadingImpl.h"
#include "MM/ByteAllocator.h"

namespace MyOS {
namespace Timer {
class ITiming;
}
}

namespace MyOS {
namespace MultiThreading {

using MyOS::Core::IComponent;
using Timer::ITimerFacility;

class MTComponent : public IComponent {

	friend class Thread; // access to iKernelMemory

	IVirtualMemory* iVirtualMemory;
	ITimerFacility* iTiming;

public:
	inline static const char* const ID() { return "4b6c1c62-ce07-4242-996e-c47bf87eefd3"; }

	MTComponent(MyOS::Core::IContainer& container) throw() INITSECTION;
	virtual myos_result_t initialize(IContext& context, NVPAIR params[]) INITSECTION;
	virtual myos_result_t deinitialize(IContext& context);


	virtual myos_result_t queryInterface(IComponent& requestor,
			const uuid_t& uuid_t, IInterface*& result);

private:
	static MTComponent* instance;

public:
	inline static MTComponent& getInstance() {
		return *instance;
	}

private:

	friend class IMultiThreadingImpl; // access interfaces
	IMultiThreadingImpl impl1;

public:
    MM::ByteAllocator allocator;

};

// within this namespace, this function can be used (GCC 3.4 does not allow operator new here)
inline void* allocate(size_t s) throw(MM::OutOfMemoryException) {
    return MTComponent::getInstance().allocator.allocate(s );
}

inline void* allocateNoThrow(size_t s) throw() {
    return MTComponent::getInstance().allocator.allocateNoThrow(s );
}

inline void deallocate(void* p, size_t s) {
    MTComponent::getInstance().allocator.deallocate(p, s );
}

}
} // namespaces


#ifndef NEW_DECLARED
#define NEW_DECLARED

// Declared in global namespace
inline void* operator new(size_t size) throw(
        MyOS::MM::OutOfMemoryException) {
    return MyOS::MultiThreading::allocate(size);
}

inline void* operator new[](size_t size) throw(
        MyOS::MM::OutOfMemoryException) {
    return MyOS::MultiThreading::allocate(size);
}

inline void operator delete(void* m, size_t s) {
    MyOS::MultiThreading::deallocate(m, s);
}

#endif  // NEW_DECLARED
#endif
