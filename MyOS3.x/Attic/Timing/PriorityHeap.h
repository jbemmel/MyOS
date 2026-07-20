// #line 1 "PriorityHeap.h"
/*
	A priorityheap class, very efficient

	There are 2 alternative implementations options:

	1. Use indices 0..MAXHEAP-1

		Parent(i)	== (i-1) >> 1
		Left(i) 	== (i << 1) + 1
		Right(i) 	== Left(i) + 1


	2. Use indices 1..MAXHEAP	(=> use index 0 for COUNT)

		Parent(i)	== (i >> 1)
		Left(i) 	== (i << 1)
		Right(i) 	== Left(i) + 1

	Option (2), although slightly more akward to define, is more efficient and therefore preferred

	- Thread safety has been moved to the client (!)

19/6/2001	Changed to 64-bit accuracy, 32+6 bits proved to be too little (max  22 minutes @ 200 MHz)

*/

#ifndef PRIORITYHEAP_H
#define PRIORITYHEAP_H

#include "types.h"
#include "debug.h"

namespace MyOS {

/// Represents an item in the heap, must be subclasses or contained
class HeapItem
{
public:
	typedef u64 PRIORITY;

   // Constructor leaves priority undefined
	inline HeapItem( ) : backpointer(0) {}

	PRIORITY getPriority() const           { return priority; }
	void setPriority( PRIORITY new_prio )  { priority = new_prio; }
	inline bool inHeap() const             { return (backpointer != 0); }

// private:
	// template <u32> friend class PriorityHeap;
	friend class PriorityHeapImp;

	PRIORITY priority;
	u32 backpointer;		// index within heap => more efficient random removal (Abort timer events)
   u32 __unused__;      // padding (added anyway by compiler), subclass may use this  
};

/// Implementation class for the Priorityheap
class PriorityHeapImp
{
public:
    // template <u32> friend class PriorityHeap;

	/// move item i (already in heap) up (not as far as possible)
	static void anti_sift( HeapItem *i, HeapItem **items );

	/// move item i (already in heap) down as far as possible
	static void sift( HeapItem *i, HeapItem **items, u32 HeapCount );

	/// update i (already in heap) up or down
	static void update( HeapItem *i, HeapItem **items, u32 HeapCount );
};

/*******************************************************
	HANDY IMPLEMENTATION DEFINITIONS
*******************************************************/
#define HEAPCOUNT (items[0])
#define HEAP ((HeapItem **) items)
#define HEAPHIGH  (HEAP[1])

// using namespace MyOS::Debug;

/// Implements storage for a N-item priorityheap
template <u32 N>
class PriorityHeap
{
public:
	PriorityHeap()		{ HEAPCOUNT = 0; }

	HeapItem *top() const
	{
		return HEAPCOUNT ? HEAPHIGH : 0;
	}

	inline size_t size() const
	{
		return HEAPCOUNT;
	}

	inline void push( HeapItem *item )
	{
		ASSERTIONv( HEAPCOUNT < N && !item->inHeap(), E_ERROR, HEAPCOUNT );

		// Set backpointer before adding...
		item->backpointer = ++HEAPCOUNT;			// only valid when using index 1..MAXHEAP
		HEAP[ HEAPCOUNT ] = item;
		PriorityHeapImp::anti_sift( item, HEAP );
	}

	inline HeapItem* pop( HeapItem *item )
	{
		if (HEAPCOUNT > 0)
		{
			ASSERTIONv( (item->backpointer > 0) && (item->backpointer <= HEAPCOUNT), E_ERROR, item->backpointer );

			HeapItem *repl = HEAP[ (HEAPCOUNT--) ];	// only valid when using index 1..MAXHEAP
			HEAP[ item->backpointer ] = repl;
			repl->backpointer = item->backpointer;
			PriorityHeapImp::sift( repl, HEAP, HEAPCOUNT );

			item->backpointer = 0;
			return item;
		} else {
			BUG( "Pop from empty heap" );
			return 0;
		}
	}

	/**
	 *	Update an item which is possibly already in the heap. If not, it will be added
	 *	@param item 				: the item to add or update
	 *	@param new_priority : the new priority for the item
	 */
	inline void update( HeapItem *item, HeapItem::PRIORITY new_priority )
	{
		HeapItem::PRIORITY old_p = item->getPriority();
		item->setPriority( new_priority );
		if (item->inHeap())					// check if i is in heap
		{
			if ( new_priority < old_p )	// smaller == higher priority
			{
				PriorityHeapImp::anti_sift( item, HEAP );
			} else {
				PriorityHeapImp::sift( item, HEAP, HEAPCOUNT );
			}

		} else push(item);			// else add it
	}

	/**
	 * Replace an item in the heap with another one, updating the heap
	 * @param i 		: the item to replace (removed from the heap)
	 * @param with  : the item to put in the heap instead of i (should not be in heap)
	 *
	 * @note Use this instead of pop(i) push(with)
	 */
	inline void replace( HeapItem *i, HeapItem *with )
	{
		HEAP[ (with->backpointer = i->backpointer) ] = with;
		PriorityHeapImp::update( with, HEAP, HEAPCOUNT );
		i->backpointer = 0;
	}

private:
	u32 items[ N+1 ];		// items[0] == count, unsigned because casting for HEAPCOUNT is more akward
};

}  // namespace

#endif	// PRIORITYHEAP_H
