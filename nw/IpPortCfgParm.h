//==============================================================================
//
//  IpPortCfgParm.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef IPPORTCFGPARM_H_INCLUDED
#define IPPORTCFGPARM_H_INCLUDED

#include "CfgIntParm.h"
#include "SysTypes.h"

namespace NodeBase
{
   class IpService;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Configuration parameter for IP ports.
//
class IpPortCfgParm : public CfgIntParm
{
public:
   //  Creates a parameter with the specified attributes, which are described
   //  in the base class constructor.  SERVICE identifies what is running on
   //  the port.
   //
   IpPortCfgParm(const char* key, const char* def,
      word* field, const char* expl, const IpService* service);

   //  Virtual to allow subclassing.
   //
   virtual ~IpPortCfgParm();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to check if VALUE (an IP port) is available.
   //
   virtual bool SetNextValue(word value) override;
private:
   //  Overridden to indicate that a cold restart is required to move an IP
   //  service to a new port.
   //
   virtual RestartLevel RestartRequired() const override { return RestartCold; }

   //  The service running on the port.
   //
   const IpService* const service_;
};
}
#endif
