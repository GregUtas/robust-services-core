//==============================================================================
//
//  CfgFlagParm.h
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
#ifndef CFGFLAGPARM_H_INCLUDED
#define CFGFLAGPARM_H_INCLUDED

#include "CfgBitParm.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Configuration parameter for flags.
//
class CfgFlagParm : public CfgBitParm
{
public:
   //  Creates a parameter with the specified attributes.
   //
   CfgFlagParm(c_string key, c_string def,
      Flags* field, FlagId fid, c_string expl);

   //  Virtual to allow subclassing.
   //
   virtual ~CfgFlagParm();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to return the parameter's current value.
   //
   bool GetValue() const override;

   //  Overridden to transfer next_ to curr_.
   //
   void SetCurr() override;

   //  Overridden to set the parameter's next value.
   //
   bool SetNextValue(bool value) override;
private:
   //  A pointer to the object that contains the flag's value.  Its
   //  MemoryType must have at least the same level of persistence
   //  as this configuration parameter
   //
   Flags* const curr_;

   //  The value to be set during an appropriate restart.
   //
   bool next_;

   //  The flag's identifier.
   //
   const FlagId fid_;
};
}
#endif
