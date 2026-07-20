/**
 * Support for (emulated) Thread Local Storage (TLS):
 * 
 * __thread int i;      // declares a variable i, with an instance _per thread_
 *
 * 
 * GCC emits 2 sections as part of the .data section:
 *  ___emutls_t.* : this contains initializer data, if not specified variables are initialized to 0 (BSS) 
 *  ___emutls_v.* : this contains variable descriptors (struct __emutls_object) 
 *                  which are fed to __emutls_get_address
 * 
 * There are 6 cases for TLS variables:
 * 1: __thread int p;
 * 2: __thread int pi = 1;
 * 3: static __thread int s;
 * 4: static __thread int si = 1;
 * 5: __thread int c __attribute__((common));
 * 6: __thread int ci __attribute__((common)) = 1;
 *
 * After testing, 5 of these (all but the initialized static 4) result in calls to __emutls_get_address
 * 
 * In case 2(pi) and 6(ci), the 'templ' attribute is filled in  
 * 
 * In case 5(c), both 'size' and 'align' are set to 0
 * 
 * Cases 5&6 emit ASM code called "global constructor keyed to ...", this calls __emutls_register_common
 * for each TLS variable. Probably expected to be invoked by linker, code is currently *discarded*
 */
#include "MT/Thread.h"
#include "MP/Process.h"
#include "Atomic/system.h"
#include "debug.h"

using MyOS::MultiThreading::Thread;
using MyOS::MultiProcessing::Process;

typedef unsigned int word __attribute__((mode(word)));
typedef unsigned int pointer __attribute__((mode(pointer)));

struct __emutls_object
{
  word size;
  word align;
  union {
    pointer offset;
    void *ptr;
  } loc;        //< Initially 0, assigned by the runtime system
  void *templ;  //< Set for TLS variable with initializer, this points to the initial value
};


/**
 * GCC emits code to invoke this function
 * This function has a built-in declaration in GCC
 * 
 */
extern "C" void *__emutls_get_address ( /*struct __emutls_object*/ void *obj )
{
    struct __emutls_object *d = (struct __emutls_object*) obj;
    //DPRINTK( "\nTLS::getAddress d=%x size=%d align=%d loc=%x templ=%x",
    //        d, d->size, d->align, d->loc.ptr, d->templ );

    Thread &cur = Thread::currentThread();
    u32 offset = d->loc.offset;
    if RARELY( offset == 0 ) {
        offset = Process::getCurrent().getNextTLSIndex( d->size, d->align );
        u32 old = cmpxchg( &d->loc.offset, 0, offset );
        if ( old!=0 ) offset = old; // RACE occurred
        return cur.initTLSVariable( offset, d->size, d->templ );
    } else {
        return cur.getTLSVariable( offset );        
    }
}

/**
 * This is invoked for TLS variables declared as "common" (__attribute__((common)))
 * Probably used for DLLs, this seems to 'patch' a descriptor
 */
extern "C" void
__emutls_register_common ( void *d, word size, word align, void *templ)
{
    struct __emutls_object *obj = (struct __emutls_object*) d;  
  // taken from http://www.koders.com/c/fidD7C65B3964CDCA681D2549712F29D38B6F3A30CF.aspx?s=cdef%3Atree
    
  if (obj->size < size) {
      obj->size = size;
      obj->templ = 0;
  }
  
  if (obj->align < align)
      obj->align = align;
  
  if (templ && size == obj->size)   /// For TLS variables with an initializer
      obj->templ = templ;
}


