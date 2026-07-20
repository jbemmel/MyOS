// NOTE: This file should be included from .cpp files, *never* from .h

/**
 * Memory management is one of the areas where MyOS differs from conventional OS
 * MyOS distinguishes four contexts for memory allocation, in order of (typical)
 * longest lifetime:
 * 
 * - Global (Should be removed?)
 * - (Current, source-defined) component
 * - (Current, runtime) process
 * - (Current, runtime) thread
 *
 * Each memory allocation must be prefixed with the right context to use. This
 * is subject to caveats, e.g. thread memory is automatically freed when the
 * thread exits.
 * 
 * The benefits of this scheme are believed to be:
 * - improved locality of reference
 * - improved accountability & (limit) control
 * - less contention / synchronization overhead (e.g. for the per thread case)
 * 
 * See http://www.ictp.trieste.it/texi/gpp/gpp_34.html#SEC29 for
 * new GCC 'new' syntax like "new {args} Object()"
 * 
 * I have looked at some available options for syntax, but all of them have
 * drawbacks:
 * 
 * operator new[] and delete[] can only be overridden by classes; you can
 * declare them in a namespace, but how do you call them there?

// According to C++ annotations at 
// http://www.icce.rug.nl/documents/cplusplus/cplusplus09.html#NEWDELETEARRAY
// the new[] and delete[] operator can only be overridden by *classes*

 */
#ifndef NEW_H
#define NEW_H

#ifdef __GNUC__

#include "Exceptions/MyOSExceptions.h"
#include "MultiThreading/IThread.h"
#include "MM/memtypes.h"

using MyOS::MM::OutOfMemoryException;

/* Declared in global namespace
inline void* ::operator new( size_t size ) throw () {
   return 0;  
}
*/

/*
inline void operator delete( void* m ) {
	BUG( "Should not be called" );
}*/

// function prototype
void operator delete( void*, size_t );

/*
namespace Process {
inline void* ::operator new( size_t size ) throw (MM::OutOfMemoryException) {
   return Process::currentProcess().allocate( size );
}
inline void* ::operator new[]( size_t size ) throw (MM::OutOfMemoryException) {
   return Process::currentProcess().allocate( size );
}
inline void ::operator delete( void* m ) {
   return Process::currentProcess().free( m ); 
}
inline void ::operator delete[]( void* m ) {
   return Process::currentProcess().free( m ); 
}
*/
   
enum E_ALLOCATION_CONTEXT {
   GLOBAL      = 0,
   //COMPONENT   = 1,
   PROCESS     = 2,
   
/**
 * Context: current thread
 * 
 * Use this context whenever possible, since it reduces contention for memory
 * allocation.
 * 
 * Some good uses:
 * - When you have a worker thread handling events handed to it by an IRQ handler
 *   -> Handler allocates memory in the context of the handler thread, thread
 *      handles them and frees the memory again when it is done with them
 * 
 * Limitations:
 * - do *not* use in IRQ context
 * - do *not* access thread-allocated storage from another thread; the thread
 *   may exit before you know it
 *   
 *   -> Exception to this rule is when you create a new thread, before you call
 *      'startThread'. Convenient way to pass parameters to the thread...
 *   -> Or more generally, when you are sure the thread has not exited
 * 
 * - freeing is encouraged but optional; exit() will free all allocated memory
 *   in an efficient way (but it will *not* call destructors of dynamically
 *   allocated objects (stackbased ones will work)...)
 */   
   THREAD      = 3,
   
   //COMPONENT_NOTHROW = 4,     // for use in IRQ handlers
};

// Usage: Object* o = new (THREAD) Object( ... )
/**
 *  NOTE: This uses autosize allocation, use matching delete!
 */
inline void* operator new( size_t s, E_ALLOCATION_CONTEXT c )
   throw (OutOfMemoryException)
{
   // DPRINTK( "\noperator new size=%u", s );
   switch(c)
   {
   case THREAD:
      return MyOS::MultiThreading::Thread::currentThread().allocate( s );
   
   default:
      return ::operator new( s );
   }   
}   

// Usage: Object* o = new (THREAD) Object[ x ]
inline void* operator new[]( size_t s, E_ALLOCATION_CONTEXT c )
   throw (OutOfMemoryException)
{
   return ::operator new(s,c);   // makes no distinction between arrays or objs
}

// Usage: ::operator delete( m, THREAD ), bit akward but alas
inline void operator delete( void* m, E_ALLOCATION_CONTEXT c )
{
   switch(c)
   {
   case THREAD:
      MyOS::MultiThreading::Thread::currentThread().free( m );
      break;
/*
   case COMPONENT:
#ifdef CURCOMPONENT   
      CURCOMPONENT::getInstance().free( m );
#else
      #error Component allocation used, but CURCOMPONENT not defined
#endif      
      break;
*/      

   default:
      ::operator delete( m );
   }   
}   

// Operator delete with given size
inline void operator delete( void* m, size_t s, E_ALLOCATION_CONTEXT c )
{
   switch(c)
   {
   case THREAD:
      // TODO: Thread does not yet support this
      MyOS::MultiThreading::Thread::currentThread().free( m /* ,s */ );
      break;
/*
   case COMPONENT:
#ifdef CURCOMPONENT      
      CURCOMPONENT::getInstance().free( m,s );
#else
      #error Component allocation used, but CURCOMPONENT not defined
#endif            
      break;
*/

   default:
      // TODO: This one is not defined anywhere
      ::operator delete( m, s );
   }   
}   

inline void operator delete[]( void* m, E_ALLOCATION_CONTEXT c ) {
   return ::operator delete(m,c); 
}

inline void operator delete[]( void* m, size_t s, E_ALLOCATION_CONTEXT c ) {
   return ::operator delete(m,s,c); 
}

// Placement new
inline void* operator new( size_t s, void* m ) { return m; }
inline void* operator new[]( size_t s, void* m ) { return m; }

/***
 * Another scheme I encountered:
 *
struct component_t {};
extern const component_t XCOMPONENT;

inline void* ::operator new( size_t s, const component_t& ) 
   throw (MM::OutOfMemoryException) {
#ifdef CURCOMPONENT
      return CURCOMPONENT::getInstance().allocate( s );
#else
      #error Component allocation used, but CURCOMPONENT not defined
#endif
}
*/

// And another
/* No longer allowed by GCC 3.4
template <typename C>
inline void* ::operator new( size_t s ) 
   throw (MM::OutOfMemoryException) 
{
   return C::getInstance().allocator.allocate( s );
}

template <typename C>
inline void* ::operator new[]( size_t s ) 
   throw (MM::OutOfMemoryException) 
{
   return C::getInstance().allocator.allocate( s );
}


template <typename C, bool nothrow>
inline void* ::operator new( size_t s ) throw()
{
   return C::getInstance().allocator.allocateNoThrow( s );
}

template <typename C>
inline void ::operator delete( void* m, size_t s ) 
{
   C::getInstance().allocator.deallocate( m, s );
}
*/

/* And yet another way
#include "Core/IComponent.h"
inline void* ::operator new( size_t s, MyOS::Core::IComponent& component ) 
   throw (MM::OutOfMemoryException) {
      return component.allocate( s );
}
*/

#endif	// __GNUC__
#endif
