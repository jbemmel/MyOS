/*

 * AtomicQueue.h

 *

 *  Created on: 13-okt-2008

 *      Author: Jeroen

 */

#ifndef ATOMICQUEUE_H_
#define ATOMICQUEUE_H_

#include "atomic32.h"

namespace MyOS
{

/**

 * An atomic non-blocking queue implementation which allows multiple
 concurrent enqueues

 * but only a single dequeue thread

 */

class AtomicQueue
{
public:

    class Item
    {
        friend class AtomicQueue;

        Item * volatile next; // must be first, 'volatile' essential! Else GCC "optimizes" dequeue()

    public:
        inline Item( ) throw() : next(0) {}
    };

    inline AtomicQueue() :
        head(0), tail((Item*) &head)
    {
    }

    inline void enqueue(Item &n)
    {
        Item *t;
        n.next = 0;
        do
        {
            t = tail;
            // t->next = &n;    // this writes head when tail==&head
        } while RARELY( ((atomic32*) &t->next)->CmpXchg_retold( 0, (u32) &n ) != 0 );

        // At this point, dequeue() could interrupt and set tail to &head. Or,
        // another enqueue() could set tail to @2

        // tail = &n;

        // If tail is still equal to t, update it. Else leave it alone
        ((atomic32*) &tail)->CmpXchg_retold( (u32) t, (u32) &n );
    }

    inline Item* dequeue()
    {
        Item *n = head;
        if (n) {
            // set n->next to != 0, to prevent enqueue from adding a new node there
            if ( !(head = (Item*) ((atomic32*)&n->next)->Replace(0xdeadbeef) )) {
                // head == 0;
                // tail = (Item*) &head; // dont overwrite new enqueue
                ((atomic32*) &tail)->CmpXchg_retold( (u32) n, (u32) &head );
            }

            // XXX There is a chance that enqueue reads n->next after returning it here!
            // If the node memory is freed, this may result in an invalid access!
            return n;
        } else {
            return 0; // empty queue
        }
    }

private:
    Item * head;
    Item * volatile tail;
};

}

#endif /* ATOMICQUEUE_H_ */

