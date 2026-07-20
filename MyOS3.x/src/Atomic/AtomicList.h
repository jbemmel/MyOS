/***************************************************************************
                          AtomicList.h  -  description
                             -------------------
    begin                : Tue Jun 11 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : j.vanbemmel@home.nl

Inspired by ideas from "A pragmatic implementation of non-blocking lists"
[ http://citeseer.ist.psu.edu/harris01pragmatic.html ]

 ***************************************************************************/
#ifndef ATOMICLIST_H
#define ATOMICLIST_H

namespace MyOS {

/**
 * Represents a sorted list which can be updated and searched atomically
 */
class AtomicList
{
public:
	class Node
	{
		friend class AtomicList;
		
		Node* next;		// comes first, compatible with AtomicQueue::Node
		unsigned key;

	public:	
		inline Node( int k ) : next(0), key(k) {}
		
		inline unsigned getKey() const { return key; }
		inline Node& setKey( unsigned k ) { key=k; return *this; }
		
		inline Node* getNext() const { return next; }
	};

   AtomicList() : dummyhead(0), dummytail(0xFFFFFFFF) {
      head = &dummyhead;
      tail = &dummytail;
      head->next = tail;
   }

	bool insert( Node& newNode );
	Node* remove( unsigned searchkey );
	Node* find( unsigned key ) const;

	Node* getHead() const { return head; }
	Node* getTail() const { return tail; }

private:
   Node* search( unsigned key, Node*& left ) const;

   Node* head;
   Node* tail;

   // two dummy nodes
   Node dummyhead, dummytail;
};

}	// namespace

#endif
