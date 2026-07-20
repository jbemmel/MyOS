/***************************************************************************
 IInterface.h  -  description
 -------------------
 begin                : Mon Mar 4 2002
 copyright            : (C) 2002 by Jeroen van Bemmel
 email                : jeroen@thebem.localdomain

 An attempt to design a COM-like component model. I dont like the 'queryInterface'
 on every interface, but reference counting is OK

 I want to add an XML description with UUID (use 'uuidgen -t'), and the means to get at this
 description (perhaps simply a public attribute)
 ***************************************************************************/
#ifndef IINTERFACE_H
#define IINTERFACE_H

#include "interfaces.h"
#include "myosresult.h"
#include "Version.h"
#include "UUID.h"
#include "buffer.h"
#include "NVPAIR.h"
#include "Name.h"
#include "Exceptions/MyOSExceptions.h"

/// The namespace for all MyOS related classes
namespace MyOS
{
namespace Core
{

class IComponent;
class IScriptable;

/**
 * The mother of all interfaces
 */
class IInterface
{
private:
    const VERSION version; //< *implementation* version
    IComponent &component; //< the component of which this interface is a part
    const char* const uuid; //< The UUID of this interface
    const char* const name; //< The (code) name of this interface
    
protected:
    inline IInterface(IComponent& c, VERSION v, const char* const u, const char* const n) throw() :
        version(v), component(c), uuid(u), name(n)
    {

    }

    // deallocation must be done in derived class' destructor
    //inline void operator delete(void*)
    //{
    //}

    // prevent code bloat! should add 'throw()', but requires destructors
    // in all derived classes
    inline ~IInterface() /* throw() */
    {
    }

    /**
     *  Callback method invoked when a component requests this interface
     *  @return @todo result ? deny access ?
     *  @note Default empty
     */
    // virtual myos_result_t onAccess( IComponent& byComponent ) { return E_MYOS_SUCCESS; }

public:
    /**
     * Returns the UUID of this interface
     */
    inline const char* const getUUID() const
    {
        return uuid;
    }

    /**
     * Returns the name of this interface
     */
    inline const char* const getName() const
    {
        return name;
    }    
    
    /**
     * @return The component by which this interface is offered
     */
    inline IComponent& getComponent() const
    {
        return component;
    }

    /** 
     * @return the version of the *implementation* 
     */
    inline VERSION getVersion() const
    {
        return version;
    }

    /**
     * Safe typecast by comparing pointer to UUID
     */
    template<typename TO> inline TO* castTo() const throw() // throw (Exceptions::CastException)
    {
        // type check by comparing UUID pointers (or address of those?)
        if (TO::ID() == this->getUUID() || strcmp( TO::ID(), this->getUUID() ) == 0 )
            return (TO*) this;

        // Some code depends on being able to use a 0 pointer return
        // throw Exceptions::CastException();
        return 0;
    }

    template<typename TO> inline TO& castToExcept() const throw(Exceptions::CastException)
    {
        TO* t = castTo<TO>();
        if ( t != 0 ) return *t;
        throw Exceptions::CastException( *this, TO::ID() );
    }
    
    /** @todo: implement */
    // virtual void release() {}

    /**
     * Returns a scriptable (decorated) version of this interface, <code>null</code> if not supported
     */
    virtual IScriptable* getScriptable() const throw()
    {
        return 0;
    }

};

// dummy implementation
// inline IInterface::~IInterface() {}

/**
 * Macros to be used for declaring interfaces. Handles basic plumbing
 */
#define SUB_INTERFACE(I,P,UUID) class I : public P { \
    public: inline static const char* const ID() { return UUID; } \
    inline static const char* const NAME() { return #I; } \
    protected: inline I(Core::IComponent& c, VERSION v) : P(c, v, ID(), NAME() ) {} \
    public:
    
#define INTERFACE(I,UUID) SUB_INTERFACE(I,Core::IInterface,UUID)    

}
} // namespace

#endif
