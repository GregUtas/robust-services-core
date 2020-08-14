//==============================================================================
//
//  CfgIntParm.h
//
//  Copyright (C) 2017  Greg Utas
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
#ifndef CFGINTPARM_H_INCLUDED
#define CFGINTPARM_H_INCLUDED

#include "CfgParm.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Configuration parameter for integer values.
//
class CfgIntParm : public CfgParm
{
public:
   //  Creates a parameter with the specified attributes.
   //
   CfgIntParm(c_string key, c_string def, word min, word max, c_string expl);

   //  Virtual to allow subclassing.
   //
   virtual ~CfgIntParm();

   //  Returns the parameter's current value.
   //
   word GetValue() const { return curr_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Sets the parameter's next value.  May be overridden to prevent
   //  invalid settings.  If so, invoke this version before returning
   //  true.
   //
   virtual bool SetNextValue(word value);

   //  Overridden to prefix the parameter's type and allowed values.
   //
   void Explain(std::string& expl) const override;

   //  Calls GetValue and maps the result to a string that corresponds
   //  to the parameter's current value.
   //
   std::string GetCurr() const override;

   //  Overridden to transfer next_ to curr_.
   //
   void SetCurr() override;

   //  Calls SetNextValue based on the value of INPUT, which is converted
   //  to an integer by reading it from an istringstream.
   //
   bool SetNext(c_string input) override;
private:
   //  The parameter's current value.
   //
   word curr_;

   //  The value to be set during an appropriate restart.
   //
   word next_;

   //  The minimum value allowed for the parameter.
   //
   word min_;

   //  The maximum value allowed for the parameter.
   //
   word max_;
};
}
#endif
