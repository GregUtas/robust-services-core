//==============================================================================
//
//  CliIntParm.h
//
//  Copyright (C) 2013-2022  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later
//  version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with RSC.  If not, see <http://www.gnu.org/licenses/>.
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
   //  Represents an integer value in parameter help text.
   //
   static fixed_string AnyIntParm;

   //  Represents a hex value in parameter help text.
   //
   static fixed_string AnyHexParm;

   //  HELP, OPT, and TAG are passed to CliParm.  MIN and MAX define the
   //  legal range for the integer value.  HEX is true if the parameter
   //  must be entered in hex.
   //
   CliIntParm(c_string help, word min, word max,
      bool opt = false, c_string tag = nullptr, bool hex = false);

   //  Virtual to allow subclassing.
   //
   virtual ~CliIntParm();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to look for an integer that lies between min_ and max_.
   //
   Rc GetIntParmRc(word& n, CliThread& cli) const override;
private:
   //  Overridden to show the range of legal values.
   //
   bool ShowValues(std::string& values) const override;

   //  Separates the minimum and maximum values in parameter help text.
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
