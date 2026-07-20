#ifndef IDRIVER_H
#define IDRIVER_H

#include "Core/IComponent.h"
#include "myosresult.h"
#include "Drivers/IDriverManager.h"
#include "Context/IContext.h"

// include these here, getting tired of redeclaring everywhere
#include "MM/IVirtualMemory.h"
#include "MM/IPhysicalMemory.h"
#include "MultiThreading/IMultiThreading.h"
#include "InterruptHandling/IInterruptHandling.h"

namespace MyOS { namespace Drivers {

using Core::IComponent;
using Core::IContainer;

// Declare these here, getting tired of redeclaring everywhere
using MM::IVirtualMemory;
using MM::IPhysicalMemory;
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
      const UUID& uuid,
      IInterface*& result
   ) { return E_MYOS_E_NOTIMPL; }

   virtual myos_result_t queryInterface(
      IComponent& requestor,
      const char* const name,
      VERSION version,
      IInterface*& result
   ) { return E_MYOS_E_NOTIMPL; }

   IDriverManager* iDriverManager;
   inline myos_result_t init( IContext& ctxt ) { 
      myos_result_t r = ctxt.lookup( IDriverManager::uuid(), (IInterface*&) iDriverManager ); 
      if (r==E_MYOS_SUCCESS) {
         if (!iDriverManager->addDriver( *this )) {
            r = E_MYOS_E_FAIL;
         }
      }
      return r;
   }

   inline void remove() {
    if (iDriverManager) {
        iDriverManager->removeDriver( *this );
        iDriverManager->release();
        iDriverManager = 0;
    }
   }

  
public:
   inline CDriverBase( IContainer& container, VERSION v, IComponent*& instance ) 
      throw() : IComponent(container,v,instance) {}

   virtual myos_result_t detectDevices( ) INITSECTION = 0;
   //virtual myos_result_t activate() = 0;	
   //virtual myos_result_t passivate() = 0;	
   //virtual myos_result_t upgrade( IDriver& oldDriver ) = 0;  
};

}} // namespace

#endif
