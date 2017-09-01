//==============================================================================
//
//  CliIncrement.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CLIINCREMENT_H_INCLUDED
#define CLIINCREMENT_H_INCLUDED

#include "Protected.h"
#include <cstddef>
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
   CliCommand* FindCommand(const std::string& text) const;

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
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Sets the corresponding member variables and adds the increment to
   //  CliRegistry.  Protected because this class is virtual.
   //
   CliIncrement(const char* name, const char* help, size_t size = 32);

   //  Adds COMM to the increment's dictionary of commands.
   //
   bool BindCommand(CliCommand& comm);
private:
   //  Overridden to prohibit copying.
   //
   CliIncrement(const CliIncrement& that);
   void operator=(const CliIncrement& that);

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
