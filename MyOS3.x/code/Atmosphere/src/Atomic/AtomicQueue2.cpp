/***************************************************************************
                          LinkQueue.cpp  -  description
                             -------------------
    begin                : Tue Feb 5 2002
    copyright            : (C) 2002 by Jeroen van Bemmel
    email                : jeroen@thebem.localdomain
 ***************************************************************************/
#include "AtomicQueue2.h"

namespace MyOS {

// AtomicQueue::Node AtomicQueue::dummy;

void enq( AtomicQueue2 *_this, void *item )
{
    ASMVOLATILE(
        "0:"
        "movl       0x4(%2), %0 \t\n"       /* p = tail */
        "movl       $0, %3      \t\n"       /* get NULL */
        "cmpxchg    %1, 0x0(%0) \t\n"       /* replace_if(==0) (p->next,q) */
        "jnz        0b          \t\n"       /* retry if failed */
        "movl       %0, %3      \t\n"       /* get p */
        "cmpxchg    %1, 0x4(%2) \t\n"       /* replace_if(==p) (tail,q) */
        :
        : "r"(0), "r"(item), "r"(_this), "a"(0)
    );
}

void* deq( AtomicQueue2 *_this )
{
    void *p;
    ASMVOLATILE(
        "0:"                                
        "movl       0x0(%0), %2     \t\n"   /* get p->next */
        "test       %2, %2          \t\n"   /* check p->next for NULL */
        "je 1f                      \t\n"   /* exit if 0 */
        "cmpxchg    %2, 0x0(%3)     \t\n"   /* replace_if(==p) (head, p->next ) */
        "jnz        0b              \t\n"   /* retry if failed, eax contains head */
        "mov        %2, %0          \t\n"   /* get q into p*/
        "jmp 2f                     \t\n"   /* finished */
        "1:"
        "movl       $0, %0          \t\n"   /* return false */
        "2:"
        : "=a"(p)
        : "0"( _this->head ), "c"(0), "d"(_this)
    );
    return p;
}

}  // namespace
