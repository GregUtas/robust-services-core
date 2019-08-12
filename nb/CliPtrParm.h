//==============================================================================
//
//  CliPtrParm.h
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
#ifndef CLIPTRPARM_H_INCLUDED
#define CLIPTRPARM_H_INCLUDED

#include "CliParm.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  CLI pointer parameter.
//
class CliPtrParm : public CliParm
{
public:
   //  HELP, OPT, and TAG are passed to CliParm.
   //
   explicit CliPtrParm(c_string help, bool opt = false, c_string tag = nullptr);

   //  Virtual to allow subclassing.
   //
   virtual ~CliPtrParm();

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to look for a valid pointer.
   //
   Rc GetPtrParmRc(void*& p, CliThread& cli) const override;
private:
   //  Overridden to show that a hex value is expected.
   //
   bool ShowValues(std::string& values) const override;
};
}
#endif
