/*
    YaQ (Yet Another Queue)

    After ideas in "Implementing Lock-Free Queues", by John Valois (page 3&4, 3rd variant)

    - Implemented as a linked list, so
        * unlimited size
        * 1 dword overhead per dword dataitem
        * no locality of reference

    - This is the blocking variant, but blocking is highly unlikely
    
   30/12/2003: Be careful when using this queue! By design the head pointer
               "points to the node that was last dequeued" (see paper),
               this means the nodes cannot be allocated on the stack!
*/

#ifndef ATOMICQUEUE_H
#define ATOMICQUEUE_H

#include "defs.h"
#include "debug.h"

namespace MyOS {

class AtomicQueue;

void enq( AtomicQueue *_this, void *item );
void *deq( AtomicQueue *_this );

class AtomicQueue
{
public:

   /**
    * A base class to be subclassed by user classes
    *
    * As alternative I first had a T* in here, but the same effect can be achieved
    * by subclassing with a class with only a T* in it
    *
    * This way, node data can be included directly or indirectly at will
    *
    * The subclass _cannot_ have any virtual functions, as this class expects
    * next at offset 0
    * 
    * NOTE: **MUST** use dynamic allocation, cannot use stack allocation !!
    */
   class Node
   {
      friend class AtomicQueue;
      Node* next;

      inline Node* getNext() const    	{ return next; }

      protected:
		inline Node() : next(0)  {}
		inline ~Node()           { next=0; }
    };

// the interface
   AtomicQueue() : head(&dummy), tail(&dummy), size(0) {}
   ~AtomicQueue() {}

    void enqueue( Node* item )  {
      // DPRINTK( "\nAtomicQueue::enqueue head=%x tail=%x head->next=%x", head, tail, head->next );
      enq( this, item );
      ++size;
      // DPRINTK( "\nafter enq: head=%x tail=%x head->next=%x", head, tail, head->next );
    }

   /**
    * Dequeues the first (head) item in this list
    *
    * @return Node dequeued, 0 if failed
    */
   Node* dequeue() {
      Node *p = (Node*) deq(this);
      if (p) {
         --size;
         // DPRINTK( "\nafter deq: head=%x tail=%x head->next=%x", head, tail, head->next );         
         return p;
      }
      return 0;
   }
   
   /**
    * O(n) function to check if a particular node is in the queue
    */
   bool contains( Node* item ) const {
      Node *n = head;
      do {
         if (n==item) return true;
      } while ( n!=tail && (n=n->next) );

      return false;
   }

    inline size_t getSize() const { return size; }

// private:
    Node *head, *tail;
    Node dummy;            // one dummy node, to reduce the number of special cases (static??)

    size_t size;           // note: my addition, not sure if thread safe...
};

}  // namespace

#endif
