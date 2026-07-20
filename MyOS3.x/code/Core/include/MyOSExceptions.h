#ifndef MYOSEXCEPTIONS_H
#define MYOSEXCEPTIONS_H

#ifndef __GNUC__
// Visual Studio does not support exception specifications
#pragma warning(disable:4290)
#endif

/**
 * Note: There is not much point to use global static exceptions to throw,
 * since '__cxa_allocate_exception' is called anyway...
 */
namespace MyOS {

namespace Core { class IInterface; }

class CastException
{
    const Core::IInterface& casted;
    const char* const castedToUUID;

public:
    inline CastException( const Core::IInterface& _casted, const char* const uuid ) throw()
        : casted(_casted), castedToUUID(uuid) {}
};

class SecurityException {};


} // namespace

#endif
