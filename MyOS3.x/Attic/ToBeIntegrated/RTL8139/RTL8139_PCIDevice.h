/***************************************************************************
 RTL8139_PCIDevice.h  -  description
 -------------------
 begin                : Sun Aug 19 2001
 copyright            : (C) 2001 by jeroen
 email                : jeroen@thebem

 Not sure if this is a suitable abstraction. What can I put in here?

 ***************************************************************************/

#ifndef RTL8139_PCIDEVICE_H
#define RTL8139_PCIDEVICE_H

#include "defs.h"
#include "../../PCI/IPCIDevice.h"

#include "RTL8139_Ethernet.h"

namespace RTL8139
{

/**
 *  This class does device state management: initialization, suspend, shutdown, resume
 */
class C_RTL8139_PCIDevice : public IPCIDevice
{
public:
    C_RTL8139_PCIDevice();

    // I_UnhandledPCIDevice functions
    virtual I_UnhandledPCIDevice& getDevice() const;
    virtual s32 list(PacketWriter& buf, u32 closeTag) const;
    virtual s32 get(const char* callarguments, PacketWriter& buf);

    virtual bool open( /* params? */);
    virtual bool close( /* params? */);

    /// Power management
    virtual bool suspend()
    {
        return false;
    }
    virtual bool resume()
    {
        return false;
    }
    virtual I_PCIDriver& getDriver() const;
    virtual I_Interface& getFunctionality()
    {
        return ethernet;
    }
    virtual const char* getInstanceID(char assign);

    /// init/deinit
    bool initialize(I_UnhandledPCIDevice& dev) INITSECTION;
    bool takeOver(C_RTL8139_PCIDevice &from);
    bool deinitialize();

private:
    /// interrupt handler
    static void handleIRQ(u32);

    /// For now, only one device is supported
    static C_RTL8139_PCIDevice *instance;

    const char deviceID[6]; // eth0, eth1, ...Merge with systemID possible ?

    // the ethernet functionality managed by this object	
    C_RTL8139_Ethernet ethernet;

    // cached pointer to info, should be reference initialized in constructor (dynamic mem)
    I_UnhandledPCIDevice *deviceInfo;
};

}
; // namespace

#endif
