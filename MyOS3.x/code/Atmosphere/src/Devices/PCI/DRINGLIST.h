#ifndef DRINGLIST_H
#define DRINGLIST_H

#include "types.h"

namespace MyOS {

///   Template for a doubly linked ring
template <typename C>
class DRINGLIST
{
public:
	/// base class for node classes (must be subclassed)
    class Node
	{
		friend class DRingList;
		Node *prev_R, *next_R;

		/// Backpointer to data object, more safe than subclassing
		C& backptr;

	public:
		inline Node( C& b) : prev_R(this), next_R(this), backptr(b) 	{}
		// ~Node() { removeR(); }	 	// NO! Node has no reference to the list it is in !

		inline C& content() const 			{ return backptr; }		
		inline Node* getPrev() const		{ return prev_R; }
		inline Node* getNext() const		{ return next_R; }

		/// Insert r before this node	
		inline void insertR( Node &r )
		{
			r.prev_R = prev_R;			
			r.next_R = this;			
			prev_R = prev_R->next_R = &r;
		}
		
		inline Node *removeR()		// returns next, 0 if next==this
		{
			Node *r = next_R;
			if (r!=this)
			{
				prev_R->next_R = next_R;
				next_R->prev_R = prev_R;				
			} else r=0;
			prev_R = next_R = this;
			return r;
		}
	};

    /// Iterator for DRINGLIST
	class Iterator
	{
	public:
		inline Iterator( DRINGLIST<C> &ring ) : start( ring.first() ), cur(start) {}

		inline operator C*() const 	{ return cur ? &cur->content() : 0; }
		inline C* operator ++() 	
            { if (cur) { cur = cur->getNext(); if (cur==start) cur=0; } return (C*) *this; }
		inline C* operator ++(int) 	
            { C* r = *this; this->operator ++(); return r; }	// postfix
		inline void restart()		{ cur=start; }

		
	private:
		Node *start, *cur;		
	};


	inline DRINGLIST() : ring(0), ringsize(0) {}
	
	inline void insert( Node &node )
	{
		if (!ring) ring = &node;
		else ring->insertR( node );
		
		++ringsize;
	}

	/// Assumes node is part of the ring!
	inline bool remove( Node &node )
	{
		ring = node.removeR();
		--ringsize;
		return true;		
	}
	
	inline Node* first() const	{ return ring; }
	inline Node* rotate()    		{ return ring ? (ring = ring->getNext()) : 0; }
	inline size_t size() const	{ return ringsize; }
	
private:
	Node *ring;		    // add: lock ?
	size_t ringsize;	// MT safe??
};

}   // namespace

#endif
