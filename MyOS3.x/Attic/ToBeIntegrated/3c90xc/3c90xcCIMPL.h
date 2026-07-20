/***************************************************************************
 C_3c90xcCIMPL.h  -  description
 -------------------
 begin                : Thu Oct 18 2001
 copyright            : (C) 2001 by Jeroen van Bemmel
 email                : jeroen@thebem.localdomain
 ***************************************************************************/
#ifndef C_3c90xcCOMPONENT_H
#define C_3c90xcCOMPONENT_H

#include "3c90xcDriver.h"

namespace _3c90xc
{

/**
 * 	This component provides support for C_3c90xc PCI network cards
 */
class C_3c90xc : public MyOS::Drivers::CDriverBase
{
    C_3c90xc(MyOS::Core::IContainer& container) throw() INITSECTION;

    virtual myos_result_t initialize(Context::IContext& context,
            NVPAIR params[]) INITSECTION;

    virtual myos_result_t deinitialize(Context::IContext& context);

    // CDriverBase
    virtual myos_result_t detectDevices() INITSECTION;

    C_3c90xcDriver driver;
};

}
; // namespace

#endif
