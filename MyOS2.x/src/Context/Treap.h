#ifndef _TREAP_H_
#define _TREAP_H_

// #include <limits.h>
#include "defs.h"
#include "Random.h"

namespace MyOS {
namespace Context {

// Treap class
//
// CONSTRUCTION: with ITEM_NOT_FOUND object used to signal failed finds
//
// ******************PUBLIC OPERATIONS*********************
// void insert( x )       --> Insert x
// void remove( x )       --> Remove x
// Comparable find( x )   --> Return item that matches x
// Comparable findMin( )  --> Return smallest item
// Comparable findMax( )  --> Return largest item
// boolean isEmpty( )     --> Return true if empty; else false
// void makeEmpty( )      --> Remove all items
// void foreach( f, p )   --> Do f(p) on each node, in sorted order


// Node and forward declaration because g++ does
// not understand nested classes.
template <class Comparable>
class Treap;

template <typename Comparable>
class TreapNode
{
   Comparable element;
   TreapNode* left;
   TreapNode* right;
   int        priority;

   TreapNode( ) : left( 0 ), right( 0 ), priority( INT_MAX ) { }
   TreapNode( const Comparable & theElement, TreapNode *lt, TreapNode *rt, int pr )
     : element( theElement ), left( lt ), right( rt ), priority( pr )
     { }

   friend class Treap<Comparable>;

   // JVB: added allocation
   inline void* operator new( size_t );
   inline void operator delete( void* b );
};

template <typename Comparable>
class Treap
{
  public:
    explicit Treap( const Comparable & notFound );
    Treap( const Treap & rhs );
    ~Treap();

    const Comparable & findMin( ) const;
    const Comparable & findMax( ) const;

    // const Comparable & find( const Comparable & x ) const;
    inline const Comparable & findFirst( const Comparable & x ) const;
    inline const Comparable & findNext( const Comparable & x, const Comparable &start ) const;

    inline bool isEmpty( ) const;

    void makeEmpty( );
    
    /// @return false iff x not unique
    bool insert( const Comparable & x );

   // Returns a copy of the element removed, or ITEM_NOT_FOUND
   Comparable remove( const Comparable & x );

   const Treap & operator=( const Treap & rhs );

    // To perform a function on every node (comparable must then be a class!)
    typedef int (Comparable::*nodefunction)( void* );
    int foreach( nodefunction f, void* param ) const;

  private:
   // for now, dont allow dynamic allocation
   inline void operator delete( void* ) {}

   const Comparable & findFrom( const Comparable & x, TreapNode<Comparable> *from ) const;

   TreapNode<Comparable> *root;
   const Comparable ITEM_NOT_FOUND;
   TreapNode<Comparable> *nullNode;
   Random randomNums;

   // Recursive routines
   bool insert( const Comparable & x, TreapNode<Comparable> * & t );
   Comparable remove( const Comparable & x, TreapNode<Comparable> * & t );
   void makeEmpty( TreapNode<Comparable> * & t );
   int execute( TreapNode<Comparable> *t, nodefunction f, void* param ) const;

   // Rotations
   void rotateWithLeftChild( TreapNode<Comparable> * & t ) const;
   void rotateWithRightChild( TreapNode<Comparable> * & t ) const;
   TreapNode<Comparable> * clone( TreapNode<Comparable> * t ) const;
};

// #include "Treap.hpp" // should be done by using class, *after* DYNAMIC_ALLOC_IMPL

}}  // namespace

#endif
