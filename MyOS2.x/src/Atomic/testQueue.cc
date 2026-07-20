#include <stdio.h>
#include "AtomicQueue.cpp"

using namespace MyOS;

class Item : public AtomicQueue::Node
{
public:
   Item() {}      
};

//
// This works, the problems start when nodes are allocated on the stack
// and disappear...
//
int main( int argc, char* argv[] )
{
   AtomicQueue queue;   

   Item i1, i2;
   
   queue.enqueue( &i1 );
   Item* d = (Item*) queue.dequeue();
   printf( "\n @i1=%x i1.dequeue=%x i1.dequeue.next=%x", &i1, d, ((int*)d)[0] );
   Item* d2 = (Item*) queue.dequeue();
   printf( "\n i2.dequeue=%x", d2 );
   return 0;
}
