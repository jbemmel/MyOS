#ifndef ICONTAINER_H
#define ICONTAINER_H

#include "IComponent.h"
#include "mem.h"    // RUNONSTACK

/// Linker constants used in implementation of create()
extern "C" u32 _DYNAMIC_ ASMNAME("_DYNAMIC_");
extern "C" u32 _DYNAMIC_END_ ASMNAME("_DYNAMIC_END_");

namespace MyOS { namespace Core {

class IContainer;

/// Header structure linked at the start of each container image
struct CONTAINERHEADER
{
   u32 magic;
   u32 version;
   size_t size;

   IContainer* instance;

   enum { MAGIC_VALUE=0xDADABEBE };
};

/**
 *      A container contains/creates 1 or more components, and manages their life cycles
 *      A container is itself a component, and a singleton
 */
class IContainer : public IComponent
{
   friend class CContainerManager;

   /// The creation function, implemented in this class
   template <class C>
   static IContainer* create(
      void* memory, const CONTAINERHEADER& header,
      const buffer& xml, IContainer& parent ) STARTSECTION;

public:
	inline void* operator new( size_t, void* m ) { return m; }
	inline void operator delete( void* ) {}

    /**
     * (Must be) overridden from IComponent, @see IComponent::initialize
     * @note All containers *must* call doInitialize(r) first!
     */
	virtual myos_result_t initialize( IContext& context, NVPAIR params[] ) INITSECTION = 0;

	// @see IInterface::get
	virtual myos_result_t get( const char* const call, NVPAIR parameters[], IO::OStream& output ) const;

   /**
    * @return XML-encoded Content of this container
    */
   virtual const buffer getContent() const = 0;

   /** Helper method to retrieve the content type from XML meta description */
   const char* getContentType() const;

   /**
    * Queries this container for a particular component, by UUID
    * @param requestor   : Component doing the lookup
    * @param uuid        : UUID to look for as defined in XML
    * @param result[out] : Result of the lookup
    * @return E_MYOS_SUCCESS if found, error otherwise
    */
   virtual myos_result_t queryComponent(
      IComponent& requestor,
      const UUID& uuid,
      IComponent*& result
   ) = 0;

   /** 
    * Looks up a particular component by name (and optionally version)
    * 
    * @param version : Version wanted, VERSION::ANY() for any version
    */
   virtual myos_result_t queryComponent(
      IComponent& requestor,
      const char* const name,
      VERSION version,
      IComponent*& result
   ) = 0;

   /**
    * This is done in the COMPONENTIMPL macro
    */
   virtual myos_result_t destroy() = 0;

   /**
    * @todo workout this one
    * This is a default implementation
    */
   virtual myos_result_t destroyComponent( IComponent& c ) = 0;

   /**
    * Provides access to the container's context
    * By default, the parent's context is returned
    */
   virtual IContext& getContext() {
      return IComponent::getContainer().getContext(); // parent
   }

   /** @see IComponent */
   virtual const buffer getCDL() const { return buffer( "TODO" ); }

protected:
   // could add constructor with buffer to IComponent
   IContainer( VERSION v, IContainer*& instance ) throw ();

   myos_result_t doDestroy( CONTAINERHEADER& h );
};

/**
 * Implementation of the static support function 'create'
 *
 * This function performs dynamic linking by updating the offsets in the GOT
 * (global offset table). It could be part of the MyOS core instead of every container
 * but in this way different kinds of containers can be themselves responsible for
 * initialization
 */
template <class C>
IContainer* // static
IContainer::create(
   void* memory, const CONTAINERHEADER& header,
   const buffer& xml, IContainer& parent ) // STARTSECTION
{
   u8* got = (u8*) &header;
   for (u32 i = (u32) &_DYNAMIC_; i < (u32) &_DYNAMIC_END_; i+=4) {
      (u32&) got[i] += (u32) got;
   }
   return new (memory) C(xml,parent);
}

#ifdef __GNUC__
#ifdef COMPONENTCOMPILATION

#define CONTAINERIMPL(C,ma,mi,args...) \
   template IContainer* IContainer::create<C>( \
      void*, const CONTAINERHEADER&, const buffer&, IContainer& p ); \
   CONTAINERHEADER _header_ MAGICSECTION = { \
      CONTAINERHEADER::MAGIC_VALUE,(ma<<16|mi), sizeof(C) }; \
   C::C( const buffer& xml, IContainer &p ) \
      : IContainer( xml, VERSION(ma,mi),p ) args {} \
   myos_result_t C::destroy() { /* RUNONSTACK(24); */ return doDestroy(_header_); }
#else
#  define CONTAINERIMPL(C,ma,mi,args...)
#endif
#endif	// __GNUC__

// Function pointer definition for the static 'create' function
typedef IContainer* (myos_create_f)(
   void* memory, const CONTAINERHEADER& header,
   const buffer& xml, IContainer& parent );

// Impl is here, to make sure IContainer is known
inline myos_result_t
IComponent::destroy()
{
   return myContainer.destroyComponent( *this );   // delegate to container
}

}}	// namespaces

#endif

