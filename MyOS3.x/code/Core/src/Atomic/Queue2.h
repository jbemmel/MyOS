/***************************************************************************
 Queue2.h  -  description
 -------------------
 begin                : Tue Jan 22 2002
 copyright            : (C) 2002 by Jeroen van Bemmel
 email                : jeroen@thebem.localdomain

 A non-blocking queue, without using any special HW synchronization instructions
 Source: "On Interrupt-Transparent Synchronization in an Embedded Object-Oriented
 Operating System" (http://citeseer.nj.nec.com/schon00interrupttransparent.html)

 TO DO: Check if actually thread safe

 ***************************************************************************/
#ifndef QUEUE2_H
#define QUEUE2_H

#include "debug.h"

namespace MyOS
{

/**
 * An atomic FIFO queue using only generic instructions
 *
 * Note: only thread-safe when enqueue() cannot be interrupted by dequeue(), e.g. when used
 * in an interrupt context. In particular, this is *not* safe for use in an SMP context
 */
class Queue
{
public:
    class Item
    {
public:
        Item * volatile next; // must be first, 'volatile' essential! Else GCC "optimizes" dequeue()
        // void *data;

        inline Item( /* void *d */) throw() :
            next(0) /*, data(d)*/
        {
        }

        /*inline ~Item() throw()
        {
            ASSERTIONv(next==0,E_ERROR,next);
        }*/
    };

    /// Initially, tail points to &head
    inline Queue() throw() :
        head(0), tail( (Item*) &head )
    {
    }

    // Static initializer
    inline void init() throw() {
        this->head = 0;
        this->tail = (Item*) &head;
    }

    /**
     * Enqueues the given item at the tail of this queue
     *
     * It is assumed enqueue takes priority over dequeue, i.e. enqueue may interrupt
     * dequeue() but not vice versa
     */
    void enqueue( Item &i ) __attribute__ ((noinline));

    Item* dequeue(); // __attribute__ ((__optimize__ ("O0")));

    inline bool isEmpty() const
    {
        return head==0;
    }

private:
    Item *head, *tail;  // order and location is important
};

} // namespace

#endif

