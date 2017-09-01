//==============================================================================
//
//  CliIntParm.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CLIINTPARM_H_INCLUDED
#define CLIINTPARM_H_INCLUDED

#include "CliParm.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  CLI integer parameter.
//
class CliIntParm : public CliParm
{
public:
   //> Indicates an integer value in parameter help text.
   //
   static fixed_string AnyIntParm;

   //> Indicates a hex value in parameter help text.
   //
   static fixed_string AnyHexParm;

   //  HELP and OPT are passed to CliParm.  MIN and MAX define the legal
   //  range for the integer parameter.  HEX is true if the parameter must
   //  be entered in hex.
   //
   CliIntParm(const char* help, word min, word max,
      bool opt = false, const char* tag = nullptr, bool hex = false);

   //  Virtual to allow subclassing.
   //
   virtual ~CliIntParm();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to look for an integer that lies between min_ and max_.
   //
   virtual Rc GetIntParmRc(word& n, CliThread& cli) const override;
private:
   //  Overridden to show the range of legal values.
   //
   virtual bool ShowValues(std::string& values) const override;

   //> Separates the minimum and maximum values in parameter help text.
   //
   static const char RangeSeparator;

   //  The minimum legal value for the integer parameter.
   //
   word min_;

   //  The maximum legal value for the integer parameter.
   //
   word max_;

   //  Whether or not the integer is to be supplied in hex.
   //
   bool hex_;
};
}
#endif
