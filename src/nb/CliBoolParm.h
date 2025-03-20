//==============================================================================
//
//  CliBoolParm.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef CLIBOOLPARM_H_INCLUDED
#define CLIBOOLPARM_H_INCLUDED

#include "CliParm.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  CLI boolean parameter.
//
class CliBoolParm : public CliParm
{
public:
   //  HELP and OPT are passed to CliParm.
   //
   explicit CliBoolParm(c_string help,
      bool opt = false, c_string tag = nullptr);

   //  Virtual to allow subclassing.
   //
   virtual ~CliBoolParm();

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to look for a boolean.
   //
   Rc GetBoolParmRc(bool& b, CliThread& cli) const override;
private:
   //  Overridden to show 't' and 'f' as acceptable inputs.
   //
   bool ShowValues(std::string& values) const override;
};
}
#endif
