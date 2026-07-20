#include "AtomicList.h"
#include "asm.h"

namespace MyOS {

#define ISMARKED(p)  ((int)p&1)
#define MARK(p)      (((int)p)|1)
#define UNMARK(p)    (((int)p)&=~1)
#define UNMARKED(p)  (Node*)(((int)p)&~1)

inline static bool CAS( AtomicList::Node** src, void* chk, int nw ) {
	bool result;
	
	// TODO: See what happens when not using 'volatile' and optimizing...
	ASMVOLATILE( "cmpxchgl %2, (%3); setz %%al"		// SMP: needs LOCK
		: "=a"(result)
		: "0"(chk),  "r"(nw), "r"(src)
		: "memory"
	);
	return result;
}


bool AtomicList::insert( Node& newNode ) {
  do {
     Node *left;
     Node *right = search( newNode.key, left );
     if (right!=tail && right->key==newNode.key) return false;
     newNode.next = right;
     if (CAS( &left->next, right, (int) &newNode)) return true;
  } while (1);
}

AtomicList::Node* 
AtomicList::remove( unsigned searchkey ) {
  Node *rightnext, *left, *right;
  do {
     right = search( searchkey, left );
     if (right==tail || right->key!=searchkey) {
     	return 0;
     }
     rightnext=right->next;
     if (!ISMARKED(rightnext)) {
        if (CAS( &right->next, rightnext, MARK(rightnext) )) break;
     }
  } while (1);

  // try to unlink the 'right' node, replacing it with its neighbour...
  if (!CAS( &left->next, right, (int) rightnext)) {
     right = search( right->key, left ); // useful ?? only iff Node* is returned i.o. bool
  }
  return right;
}

AtomicList::Node* 
AtomicList::find( unsigned key ) const
{
	Node* left=0;
	Node* right = search( key, left );
	return (right!=tail && right->key==key) ? right : 0;
}

// private:
AtomicList::Node* 
AtomicList::search( unsigned key, Node*& left ) const
{
  Node* leftNext = 0;
  do {

     // 1. Find right&left node
     Node* t=head;
     Node* tNext = head->next;
     do {
        if (!ISMARKED(tNext)) {
           left = t;
           leftNext = tNext;
        }
        t = UNMARKED(tNext);
        if (t==tail) break;
        tNext = t->next;
     } while (ISMARKED(tNext) || (t->key < key));
     
     Node* right = t;

     // Check adjacency and remove on or more marked nodes (combined steps)
     if ((leftNext==right)||(CAS( &left->next, leftNext, (int) right))) {
     	if ((right==tail) || !ISMARKED(right->next)) 
			return right;
     }
  } while (1);
}

}	// namespace
