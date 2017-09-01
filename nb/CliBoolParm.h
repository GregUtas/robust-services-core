//==============================================================================
//
//  CliBoolParm.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CLIBOOLPARM_H_INCLUDED
#define CLIBOOLPARM_H_INCLUDED

#include "CliParm.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  CLI bool parameter.
//
class CliBoolParm : public CliParm
{
public:
   //  HELP and OPTIONAL are passed to CliParm.
   //
   explicit CliBoolParm(const char* help,
      bool opt = false, const char* tag = nullptr);

   //  Virtual to allow subclassing.
   //
   virtual ~CliBoolParm();

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to look for a boolean.
   //
   virtual Rc GetBoolParmRc(bool& b, CliThread& cli) const override;
private:
   //  Overridden to show 't' and 'f' as acceptable inputs.
   //
   virtual bool ShowValues(std::string& values) const override;

   //> Indicates a boolean value in parameter help text.
   //
   static fixed_string AnyBoolParm;
};
}
#endif
