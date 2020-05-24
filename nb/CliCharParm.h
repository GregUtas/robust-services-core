//==============================================================================
//
//  CliCharParm.h
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
#ifndef CLICHARPARM_H_INCLUDED
#define CLICHARPARM_H_INCLUDED

#include "CliParm.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  CLI character parameter.
//
class CliCharParm : public CliParm
{
public:
   //  HELP, OPT, and TAG are passed to CliParm.  CHARS lists the
   //  characters that are valid for this parameter.
   //
   CliCharParm(c_string help, c_string chars,
      bool opt = false, c_string tag = nullptr);

   //  Virtual to allow subclassing.
   //
   virtual ~CliCharParm();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to look for a valid character.
   //
   Rc GetCharParmRc(char& c, CliThread& cli) const override;
private:
   //  Overridden to show the acceptable character inputs.
   //
   bool ShowValues(std::string& values) const override;

   //  Separates valid input characters in parameter help text.
   //
   static const char CharSeparator;

   //  The characters that are valid for this parameter.
   //
   fixed_string chars_;
};
}
#endif
