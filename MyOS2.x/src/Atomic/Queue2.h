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

namespace MyOS {

class Queue
{
public:
	class Item
	{
	public:
		Item *next;		// must be first
		// void *data;

		Item( /* void *d */ ) : next(0) /*, data(d)*/ {}

		~Item() { ASSERTIONv(next==0,E_ERROR,next); }
	};

	/// Initially, tail points to &head
	Queue() : head(0), tail( (Item*) &head ) {}

	void enqueue( Item &i );
	Item* dequeue();	

   bool isEmpty() const { return head==0; }

private:
	Item *head, *tail;		// order and location is important
};

}  // namespace

#endif

