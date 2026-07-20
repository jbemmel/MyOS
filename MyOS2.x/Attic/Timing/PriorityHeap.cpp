#include "PriorityHeap.h"

namespace MyOS {

/*******************************************************
	IMPLEMENTATION DEFINITIONS
*******************************************************/
#define PARENT(i) (i>>1)
#define LEFT(i)   (i<<1)
#define LOWESTINDEX (1)
	
void 
PriorityHeapImp::anti_sift( HeapItem *i, HeapItem **items )		// move item i up (not as far as possible)
{
	u32 index = i->backpointer;
	while (index >= LEFT(LOWESTINDEX))
	{
		u32 parent = PARENT(index);
		if (HEAP[parent]->priority > HEAP[index]->priority)	// low values imply high priority !
		{
			HeapItem *temp = HEAP[index];
			HEAP[index] = HEAP[parent];
			HEAP[parent] = temp;
			HEAP[index]->backpointer = index;		// index == value, update swapped item only
			index = parent;
		} else break;					
	}
	i->backpointer = index;
}

// move item i down as far as possible	
void 
PriorityHeapImp::sift( HeapItem *i, HeapItem **items, unsigned HeapCount )	
{
	u32 index = i->backpointer;
	while (index <= PARENT(HeapCount))
	{
		u32 child = LEFT(index), largest = index;
		
		// ASSERT( child && largest && (largest <= HeapCount ) && HEAP[largest] );

		/// Test left child		
		if ((child <= HeapCount) && (HEAP[largest]->priority >= HEAP[child]->priority))
		{
			largest = child;
		}

		/// Test right child (at index + 1)
		if ((++child <= HeapCount) && (HEAP[largest]->priority >= HEAP[child]->priority))
		{
			largest = child;
		}

		/// See if 'largest' was changed, if not exit
		if (largest != index)
		{
			// ASSERT( (index <= HeapCount) && (largest <= HeapCount) );
		
			HeapItem *temp = HEAP[index];
			HEAP[index] = HEAP[largest];
			HEAP[largest] = temp;
			HEAP[index]->backpointer = index;	// index == value, update swapped item only 
			index = largest;
		} else break;
	}

	/// Update new place in heap
	i->backpointer = index;			
}

void
PriorityHeapImp::update( HeapItem *i, HeapItem **items, u32 HeapCount )	// update item i
{
	u32 index = i->backpointer;
	if (( index==LOWESTINDEX ) || (HEAP[PARENT(index)]->priority < i->priority))
	{
		sift( i, items, HeapCount );		// move down (i has lower prio)
	} else if (index > LOWESTINDEX) {
		anti_sift( i, items );					// move up (i has higher prio)
	}
}

}  // namespace
