//==============================================================================
//
//  CliCommandSet.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CLICOMMANDSET_H_INCLUDED
#define CLICOMMANDSET_H_INCLUDED

#include "CliCommand.h"
#include <cstddef>

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
   virtual bool BindParm(CliParm& parm) override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  HELP, COMM (the command's name), and SIZE (the maximum number
   //  of parameters that follow the command) are passed to CliCommand.
   //  Protected because this is class is virtual.
   //
   CliCommandSet(const char* comm, const char* help, size_t size = 32);
private:
   //  Used while parsing the command.  INDEX is the offset within
   //  Parms() where a valid subcommand was found.
   //
   static void DescendTo(CliCookie& cookie, size_t index);

   //  Overridden to find and invoke a subcommand.
   //
   virtual word ProcessCommand(CliThread& cli) const override;
};
}
#endif
