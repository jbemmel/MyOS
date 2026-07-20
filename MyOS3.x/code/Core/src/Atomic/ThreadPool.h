/*
 * ThreadPool.h
 *
 *  Created on: 8-okt-2008
 *      Author: Jeroen
 */

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

namespace MyOS {

/**
 * A concurrent LIFO thread pool with a maximum of N entries
 */
template <u32 N = 8>
class ThreadPool
{
    using MultiThreading::Thread;

    Thread* pool[N];
    atomic32 head;

    inline ThreadPool( ) throw()
    : head(0)
    {
        // initially empty, head==0
        memset_aligned( pool, 0, sizeof(pool) );
    }

    /**
     * Removes a thread from the head of this pool, <code>0</code> if empty
     *
     * @return Thread
     */
    inline Thread* getThread() {
        u32 h, next;
        do {
            h = head;
            if (h==0) return 0; // or throw?
            next = pool[ h & 0xfff ];
        } while ( RARELY(head.CmpXchg_retold(h,next) != h) );

        // Thread is allocated on 1 page, use that to calculate Thread object offset
        return (Thread*) ((h&0xfffff000) + 0x1000 - sizeof(Thread));
    }

    /**
     * Adds a thread to the head of this pool
     */
    inline void addThread( Thread *t ) {
        u32 h, next_head;
        do {
            h = head;
            next_head = ((u32)t&0xfffff000) | (h&0xfff);
        } while ( RARELY(head.CmpXchg_retold(h,next_head) != h) );
    }

};

}

#endif /* THREADPOOL_H_ */
