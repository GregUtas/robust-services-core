//==============================================================================
//
//  CfgBoolParm.h
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
#ifndef CFGBOOLPARM_H_INCLUDED
#define CFGBOOLPARM_H_INCLUDED

#include "CfgBitParm.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Configuration parameter for boolean values.
//
class CfgBoolParm : public CfgBitParm
{
public:
   //  Creates a parameter with the specified attributes.
   //
   CfgBoolParm(c_string key, c_string def, c_string expl);

   //  Virtual to allow subclassing.
   //
   virtual ~CfgBoolParm();

   //  Overridden to return the parameter's current value.
   //
   bool CurrValue() const override { return curr_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to return the parameter's next value.
   //
   bool NextValue() const override { return next_; }

   //  Overridden to transfer next_ to curr_.
   //
   void SetCurr() override;

   //  Overridden to set the parameter's next value.
   //
   bool SetNextValue(bool value) override;
private:
   //  The parameter's current value.
   //
   bool curr_;

   //  The value to be set during an appropriate restart.
   //
   bool next_;
};
}
#endif
