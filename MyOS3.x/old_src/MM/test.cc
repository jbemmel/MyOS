#include <stdio.h>

#define DPRINTK printf
//#define OLD_FREE   // new not correct yet!
#include "BitBuddyAllocator.hpp"   // hpp !
#undef DPRINTK

#include <string.h>  // memset
#include <stdlib.h>   // rand
#include <time.h>

using namespace MyOS::MM;

int main( int argc, char* argv[] )
{
   BitBuddyAllocator<30,12> *allocator = new BitBuddyAllocator<30,12>();
   allocator->init( );
   allocator->free( 1, 18 );  // start with 1Gb
   allocator->initDone( );
   printf( "\nSizeof(allocator)=%d #units=%d", 
      sizeof( *allocator ), allocator->getAvailableUnits() );
   // allocator->showMRUUnits( );

  /* Seed the random-number generator with current time so that
   * the numbers will be different every time we run.
   */
   srand( (unsigned)time( NULL ) );

   u32 fragmentation, distribution;
   for (int r=0; r<10; ++r) {

     #define N 1000

      unsigned frames[ N ];
      unsigned orders[ N ];

      for (int i=0; i<N; ++i) {
         orders[i] = 0; // rand() & 0xF;  // 0..15
         frames[i] = allocator->allocate( orders[i] );
         // allocator->showMRUUnits();
      }
      // break;
      // allocator->showMRUUnits();

      // reverse dealloc ensures clean allocator         
      for (int i=N-1; i>=0; --i) {
      //for (int i=0; i<N; ++i) {
         allocator->free( frames[i], orders[i] );
         // printf( "\nAFTER i=%d", i ); allocator->showMRUUnits();
      }
   }
      
   allocator->showMRUUnits();
   allocator->getMetrics(fragmentation,distribution);
   printf( "\nFrag=%d Dist=%d", fragmentation, distribution );
   allocator->compact();
   printf( "\n\nAFTER compact:" );
   allocator->showMRUUnits();
   allocator->getMetrics(fragmentation,distribution);
   printf( "\nFrag=%d Dist=%d", fragmentation, distribution );

   delete allocator;
   return 0;
}
