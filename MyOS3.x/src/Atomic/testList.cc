#include <stdio.h>
#include "AtomicList.cpp"

using namespace MyOS;

class Item : public AtomicList::Node
{
public:
   Item( int k ) : AtomicList::Node(k)  {}
};

int main( int argc, char* argv[] )
{
   AtomicList list;   

   Item i1(1), i2(2);
   
   bool in = list.insert( i1 );
   printf( "\ni1=%x i2=%x After insert(1) head=%x tail=%x head->next=%x n->n=%x", 
   	&i1, &i2, list.getHead(), list.getTail(), list.getHead()->getNext(), list.getHead()->getNext()->getNext() );
   bool in2 = list.insert( i2 );
   printf( "\nAfter insert(2) head=%x tail=%x head->next=%x n->n=%x", 
		list.getHead(), list.getTail(), list.getHead()->getNext(), list.getHead()->getNext()->getNext() );
   Item* d = (Item*) list.find(2);
   printf( "\n @i1=%x in=%x in2=%x find(2)=%x", &i1, in, in2, d );
   Item* d2 = (Item*) list.remove(2);
   printf( "\nAfter remove(2) head=%x tail=%x head->next=%x n->n=%x", 
		list.getHead(), list.getTail(), list.getHead()->getNext(), list.getHead()->getNext()->getNext() );   
   Item* d1 = (Item*) list.remove(1);
   printf( "\nAfter remove(1)=%x remove(2) was %x", d1, d2 );
   return 0;
}
