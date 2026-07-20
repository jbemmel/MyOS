#ifndef ISCRIPTABLE_H
#define ISCRIPTABLE_H

#include "IInterface.h"

/*
#include "myosresult.h"
#include "Version.h"
#include "UUID.h"
#include "buffer.h"
#include "NVPAIR.h"
#include "Name.h"
*/

namespace MyOS { 

namespace IO {
    class IStream;
    class OStream;
}
	
namespace Core {

class IComponent;

/**
 * Aspect of an IIterface that provides an XML API description of (a subset of) the
 * support operations, and methods to script these methods.
 *
 * Adding XML & scripting to every interface is heavy and tedious, hence this *optional*
 * "side" interface associated with an IInterface
 */
class IScriptable : public IInterface
{
protected:
    inline IScriptable( IComponent& c, VERSION v, const char* const name ) throw()
	    : IInterface(c,v,ID(),name)
    {

    }

public:
    inline static const char* const ID() { return "1a3306e3-d458-43ef-b172-457173bb9919"; }
	
	/**
	 * @return Description of this interface in IDL (XML)
	 */
	virtual const buffer getIDL() const throw() = 0;
   
   /**
    * Gets data from the interface
    * @param call       : Call to make, "" (empty) or "IDL" will return idl
    * @param parameters : (optional) parameters to the call, null if none
    * @param output     : output stream to receive data
    */
	virtual myos_result_t get( const char* const call, 
		NVPAIR parameters[], IO::OStream& output ) const = 0;

	/**
	 * Puts data to this interface
	 *
	 * The semantics of this operation depend entirely on the 'call' and parameters
	 */
	virtual myos_result_t put( const char* const call, 
		NVPAIR parameters[], IO::IStream& input ) 
	{
		return E_MYOS_E_NOTIMPL;
	}
};

}}	// namespace

#endif
