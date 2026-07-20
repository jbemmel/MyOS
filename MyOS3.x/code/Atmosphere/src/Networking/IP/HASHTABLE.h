/***************************************************************************
                          Hashtable.h  -  description
                             -------------------
    begin                : Sun Nov 11 2001
    copyright            : (C) 2001 by jeroen
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "debug.h"

namespace MyOS {
namespace Util {

/// A fall-through hashtable implementation
/**
 A simple fall-through hashtable implementation: Use hashvalue as starting 
 index, then do a linear search until either the entry is found or there is 
 no entry stored

 The hashtable template requires 2 classes: a <KEY> class and an <ELEMENT> class
 <ELEMENT> objects are stored in the hashtable, <KEY> objects are used to look them up

 The <KEY> part will often be a part of the <ELEMENT>, e.g. for a hashtable of Sockets with
 an IP header as key the Socket class would store data like remoteIP, protocol, UDP/TCP portnumbers

 @todo: Perhaps create a HastableIMPL class to reduce code bloat. Perhaps the "matches" function
 could be defined to be memcmp then

 @note Cannot add a CONDITION object to hash(), since an objects location in the table
  should be fixed at insertion time.
**/
template <class KEY, class ELEMENT, size_t LOG2MAXSIZE>
class HASHTABLE
{
private:
	enum { MAXSIZE = (1<<LOG2MAXSIZE), MASK = MAXSIZE-1, WOULD_BE = 0x80000000 };

	/// Returns the index of the key
	/// MAXSIZE when the entire table has been processed
	/// index | WOULD_BE if entry is zero
	u32 findIndex( const KEY &key ) const
	{
		u32 hashkey = key.hash() & MASK;
		u32 index = hashkey;
		do {
			register ELEMENT* result = elements[index];
			if (result) {
				if (key.matches(*result)) return index;
			} else return index | WOULD_BE;		// index of where it *should* be
			index = ++index & MASK;
		} while (index != hashkey);
		return MAXSIZE;		
	}

public:
	HASHTABLE() : count(0) {
		memset_aligned( elements, 0, sizeof(elements) );
	}

	// XX Could use element.getKey instead of parameter
	bool put( const KEY &key, ELEMENT &element )
	{
		if (count<MAXSIZE) {
			u32 index = findIndex( key );
			if (index & WOULD_BE) {				
				++count;
				elements[index&~WOULD_BE] = &element;
				return true;
			} else DPRINTK( "\nput failed, existing item index=%x", index );
		} else {
			PRINTK( "\nHashtable full, count: %d", count );
		}
		return false;	// full or already in table
	}

	/// Typically O(1), but O(n) worst case
	ELEMENT* get( const KEY& key ) const
	{
		u32 index = findIndex(key);
		return (index<MAXSIZE) ? elements[index] : 0;	// WOULD_BE makes it > MAXSIZE
	}

	ELEMENT* remove( const KEY& key )
	{
		ELEMENT *element=0;
		u32 index = findIndex(key);
		if (index<MAXSIZE) {
			element = elements[index];
			elements[index]=0;
			--count;

			// move up all elements that previously suffered from collissions, or
			// else they can no longer be found!
			u32 hash = key.hash();
			for (u32 i=index+1,prev=index; i!=index; prev=i,++i) {
	            i &= MASK;
    	        ELEMENT *c = elements[i];
	            if (c && c->getKey().hash()==hash) {	// requires getKey()
	               elements[prev] = c;
	               elements[i] = 0;
	            } else break;
			}

			// alternative: mark collissions upon put() in bit 0 -> no getKey()
		}
		return element;
	}

   /// Iterator for the hashtable entries
	class Iterator
	{
	private:
		const HASHTABLE& table;
		u32 curindex;
	public:
		Iterator( const HASHTABLE &t ) : table(t), curindex(0) {}
		
		inline ELEMENT* next() {
			while (curindex<HASHTABLE::MAXSIZE) {
				++curindex;
				if (table.elements[curindex-1])
					return table.elements[curindex-1];
			}
			return 0;
		}
		
		inline bool hasNext() const { return curindex<HASHTABLE::MAXSIZE; }
	};

	friend class Iterator;	
	inline Iterator iterator() const { return Iterator(*this); }

private:
	ELEMENT* elements[MAXSIZE];
	size_t count;
};

}} // namespace

#endif
