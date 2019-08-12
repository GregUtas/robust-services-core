//==============================================================================
//
//  CfgBitParm.h
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
#ifndef CFGBITPARM_H_INCLUDED
#define CFGBITPARM_H_INCLUDED

#include "CfgParm.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Configuration parameter virtual base class for bools and flags.
//
class CfgBitParm : public CfgParm
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~CfgBitParm();

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Creates a parameter with the specified attributes.  Protected
   //  because this class is virtual.
   //
   CfgBitParm(c_string key, c_string def, c_string expl);

   //  Returns the parameter's current value.
   //
   virtual bool GetCurrValue() const = 0;

   //  Overridden to prefix the parameter's type and allowed values.
   //
   void Explain(std::string& expl) const override;

   //  Calls GetCurrValue and maps the result to ValidTrueChars[0]
   //  or ValidFalseChars[0].
   //
   std::string GetCurr() const override;

   //  Calls SetNextValue based on the value of INPUT, which must
   //  be a character in ValidTrueChars or ValidFalseChars.
   //
   bool SetNext(const std::string& input) override;
private:
   //  Sets the parameter's next value.  Returns false if VALUE
   //  is invalid.
   //
   virtual bool SetNextValue(bool value) = 0;

   //  Returns a string containing the characters that set a
   //  configuration parameter to "true".
   //
   static fixed_string ValidTrueChars();

   //  Returns a string containing the characters that set a
   //  configuration parameter to "false".
   //
   static fixed_string ValidFalseChars();
};
}
#endif
