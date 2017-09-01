//==============================================================================
//
//  CliPtrParm.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CLIPTRPARM_H_INCLUDED
#define CLIPTRPARM_H_INCLUDED

#include "CliParm.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  CLI pointer parameter.
//
class CliPtrParm : public CliParm
{
public:
   //  HELP and OPTIONAL are passed to CliParm.
   //
   explicit CliPtrParm(const char* help,
      bool opt = false, const char* tag = nullptr);

   //  Virtual to allow subclassing.
   //
   virtual ~CliPtrParm();

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to look for a valid pointer.
   //
   virtual Rc GetPtrParmRc(void*& p, CliThread& cli) const override;
private:
   //  Overridden to show that a hex value is expected.
   //
   virtual bool ShowValues(std::string& values) const override;
};
}
#endif
