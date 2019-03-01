//==============================================================================
//
//  CliIncrement.h
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
#ifndef CLIINCREMENT_H_INCLUDED
#define CLIINCREMENT_H_INCLUDED

#include "Protected.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "RegCell.h"
#include "Registry.h"
#include "SysTypes.h"

namespace NodeBase
{
   class CliCommand;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  A set of related CLI commands.
//
class CliIncrement : public Protected
{
public:
   //  Removes the increment from CliRegistry.  Virtual to allow subclassing.
   //
   virtual ~CliIncrement();

   //  Used by the CLI to search for a command whose name matches TEXT.
   //
   CliCommand* FindCommand(const std::string& comm) const;

   //  Displays a one-line summary of the increment's purpose if LEVEL is 0.
   //  If LEVEL is 1, displays a one-line summary of each of the increment's
   //  commands.  If LEVEL is 2, displays an overview (if any) followed by
   //  all parameters for each command.  Returns 0.
   //
   word Explain(std::ostream& stream, int level) const;

   //  Invoked when the CLI enters the increment.  Allocates any resources
   //  required by the increment when it is active.
   //
   virtual void Enter();

   //  Invoked when the CLI exits the increment.  Frees any resources that
   //  were allocated by Enter.
   //
   virtual void Exit();

   //  Returns the increment's name.
   //
   const char* Name() const { return name_; }

   //  Returns the offset to iid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Sets the corresponding member variables and adds the increment to
   //  CliRegistry.  Protected because this class is virtual.
   //
   CliIncrement(const char* name, const char* help, uint32_t size = 32);

   //  Adds COMM to the increment's dictionary of commands.
   //
   bool BindCommand(CliCommand& comm);
private:
   //  Deleted to prohibit copying.
   //
   CliIncrement(const CliIncrement& that) = delete;
   CliIncrement& operator=(const CliIncrement& that) = delete;

   //  The increment's index in CliRegistry.
   //
   RegCell iid_;

   //  The increment's name.
   //
   const char* name_;

   //  The increment's purpose.
   //
   const char* help_;

   //  The increment's commands.
   //
   Registry< CliCommand > commands_;
};
}
#endif
