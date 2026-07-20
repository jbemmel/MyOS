#ifndef HEAPITEM_H
#define HEAPITEM_H

namespace MyOS {

/// Represents an item in the heap, must be subclasses or contained
class HeapItem
{
public:
	typedef u64 priority_t;

	// Highest possible priority, items with this priority will be on top
	enum { HIGHEST_PRIORITY = 0 };


   // Constructor leaves priority undefined
	inline HeapItem( ) : backpointer(0) {}

	inline priority_t getPriority() const              { return priority; }
	inline void setPriority( priority_t new_prio )     { priority = new_prio; }
	inline bool inHeap() const                         { return (backpointer != 0); }

// private:
	// template <u32> friend class PriorityHeap;
	friend class PriorityHeapImp;

	priority_t priority;
	u32 backpointer;	// index within heap => more efficient random removal (Abort timer events)
};

}

#endif
