//==============================================================================
//
//  CliCommandSet.h
//
//  Copyright (C) 2013-2020  Greg Utas
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
#ifndef CLICOMMANDSET_H_INCLUDED
#define CLICOMMANDSET_H_INCLUDED

#include "CliCommand.h"
#include <cstdint>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  A set of related CLI commands.
//
class CliCommandSet : public CliCommand
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~CliCommandSet();

   //  Adds COMM to the command set's dictionary of commands.
   //
   bool BindCommand(CliCommand& comm);

   //  Overridden to prevent anything other than a command from
   //  being added as a parameter.
   //
   bool BindParm(CliParm& parm) override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  HELP, COMM (the command's name), and SIZE (the maximum number
   //  of parameters that follow the command) are passed to CliCommand.
   //  Protected because this is class is virtual.
   //
   CliCommandSet(c_string comm, c_string help, uint32_t size = 32);
private:
   //  Overridden to display the subcommands as if they were grouped
   //  within a CliTextParm.
   //
   word ExplainCommand(std::ostream& stream, bool verbose) const override;

   //  Overridden to find and invoke a subcommand.
   //
   word ProcessCommand(CliThread& cli) const override;
};
}
#endif
