#ifndef IDRIVER_H
#define IDRIVER_H

#include "interfaces.h"
#include "IComponent.h"
#include "myosresult.h"
#include "IDriverManager.h"
#include "IContext.h"

// include these here, getting tired of redeclaring everywhere
#include "IVirtualMemory.h"
#include "IMultiThreading.h"
#include "IInterruptHandling.h"

namespace MyOS { namespace Drivers {

using Core::IComponent;
using Core::IContainer;

// Declare these here, getting tired of redeclaring everywhere
using MM::IVirtualMemory;
using MultiThreading::IMultiThreading;
using InterruptHandling::IInterruptHandling;

/// The parent class of all drivers
/**
 * Drivers are a special kind of components, with particular features
 * and uses:
 * - They are demand-loaded based on discovered devices
 * - They are often upgraded, due to hardware updates and/or bug fixes
 * - They should be hot-upgradeable
 *
 */
class CDriverBase : public IComponent {

protected:
   friend class IDriverManagerImpl;
//   virtual ~IDriver() = 0;

   virtual myos_result_t queryInterface(
      IComponent& requestor,
      const uuid_t &uuid,
      IInterface*& result
   ) { return E_MYOS_E_NOTIMPL; }

   IDriverManager* iDriverManager;

   inline myos_result_t init( Context::IContext& ctxt ) {
       iDriverManager = (IDriverManager*) &ctxt.lookup( myos_name_t(IDriverManager::ID()) );
       if (!iDriverManager->addDriver( *this )) {
            return E_MYOS_E_FAIL;
       }
       return E_MYOS_SUCCESS;
   }

   inline void remove()
   {
        if (iDriverManager) {
            iDriverManager->removeDriver( *this);
            // iDriverManager->release();
            iDriverManager = 0;
        }
   }


public:
   inline CDriverBase( IContainer& container, VERSION v, const char* const u, IComponent*& instance )
      throw() : IComponent(container,v,u,instance) {}

   virtual myos_result_t detectDevices( ) INITSECTION = 0;
   //virtual myos_result_t activate() = 0;
   //virtual myos_result_t passivate() = 0;
   //virtual myos_result_t upgrade( IDriver& oldDriver ) = 0;
};

}} // namespace

#endif
