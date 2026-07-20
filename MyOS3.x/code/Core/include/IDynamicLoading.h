#ifndef IDynamicLoading_H
#define IDynamicLoading_H

#include "IInterface.h"

using MyOS::Core::IInterface;

namespace MyOS {
namespace DynamicLoading {

/**
 * Interface for dynamically loading ELF modules
 */
INTERFACE( IDynamicLoading, "d1f3cb98-52e4-4579-b4da-e359c90f314a" )

    /**
     * Dynamically loads an ELF module and initializes it
     *
     * @param elfHdr  - pointer to ELF header in memory
     * @param context - context to pass to initModule()
     *
     */
    virtual bool loadModule( const u8 * const elfHdr, Context::IContext &context ) = 0;

};

}
} // namespaces
#endif
