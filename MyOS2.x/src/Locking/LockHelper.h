#ifndef LOCKHELPER_H
#define LOCKHELPER_H

#include "spinlock.h"

namespace MyOS { namespace Locking {

/**
 * Helper class to automatically lock a spinlock (IRQ save) upon entry,
 * and free it upon exit
 */ 
class LockHelper {

   // This class is meant to be instantiated on the stack!
   void* operator new( size_t );
   void operator delete( void* );
   
   spinlock_t& lock;
   u32 flags;
public:    
   inline LockHelper( spinlock_t& l ) : lock(l) { 
      spin_lock_irqsave(&lock,flags); 
   }
   inline ~LockHelper() { 
      spin_unlock_irqrestore(&lock,flags); 
   }
 
};
 
}} // namespace

#endif
