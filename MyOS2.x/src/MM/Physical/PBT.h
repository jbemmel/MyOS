#ifndef PBT_H
#define PBT_H

#include "PageDirectory.h"

namespace MyOS {
namespace MM {

    /// Represents the pagedirectory base table
    class PBT : public PageDirectory
    {
      void init( physadr_t mapping );
    public:
        // just below the pagetables, the last PDT *IS* the PBT
        static PBT& getCurrentPBT() {
            return ((PBT*)BASEADDRESS)[ 1023 ];
        }

      /**
       * Constructor, called once for each new process
       * @note when called, (*this) is *not* the current PBT!
       */
      PBT( physadr_t mapping );

      /// returns a reference to the directory, allocates it if needed
      static PageDirectory& getDirectory( linadr_t forAdr );

      static inline bool isMapped( const PageDirectory& dir ) {
         return getCurrentPBT().entries[ getPDTIndex(&dir) ].isMapped();               
      }
      
      static physadr_t getMapping( const PageDirectory& dir ) {
         return getCurrentPBT().entries[ getPDTIndex(&dir) ].getMapping();        
      }
    };

}} // namespace

#endif
