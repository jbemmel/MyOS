/***************************************************************************
                          Queue2.cpp  -  description
                             -------------------
    begin                : Tue Jan 22 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "Queue2.h"

#include "debug.h"

namespace MyOS {

void
Queue::enqueue( Item &i )
{
	ASSERTION( tail, E_ERROR );   
   
	i.next = 0;
	Item *last = tail;  // remember original tail
	tail = &i;          // insertion is here

	// check for any added nodes, then add i to the last one
	while (last->next) last=last->next;
 
	// At this point another process can still enqueue a node.
	// However, it will not overwrite 'last->next', but it will be added to the
	// tail of 'i'
 
	last->next = &i;
}

Queue::Item*
Queue::dequeue()
{
	Item *i = head;
	if (i) {
	 	if (!((head=i->next)))        // queue now empty ?
		{
			tail = (Item*) &head;      // yes, restore initial state
       
         // Solve a (rare) case where enqueue overlaps dequeue, and adds nodes
         // to the end of the node being dequeued.
			if (Item *lost = i->next) {
				Item *temp;

				do {
					temp = lost->next;   // safe this, set to 0 by enqueue
					enqueue( *lost );				
				} while ((lost=temp));
			}
		}
		i->next=0;
	}
	return i;
}

}
