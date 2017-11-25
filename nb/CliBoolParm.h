//==============================================================================
//
//  CliBoolParm.h
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
