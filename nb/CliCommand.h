//==============================================================================
//
//  CliCommand.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CLICOMMAND_H_INCLUDED
#define CLICOMMAND_H_INCLUDED

#include "CliText.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  CLI command.
//
class CliCommand : public CliText
{
   friend class CliThread;
public:
   //> Field width for command names in help text.
   //
   static const int CommandWidth;

   //> Separates the name of an increment from a command in help text.
   //
   static const char CommandSeparator;

   //  Virtual to allow subclassing.
   //
   virtual ~CliCommand();

   //  Overridden to look for an integer as the next parameter.
   //
   virtual Rc GetIntParmRc(word& n, CliThread& cli) const override;

   //  Overridden to look for a boolean as the next parameter.
   //
   virtual Rc GetBoolParmRc(bool& b, CliThread& cli) const override;

   //  Overridden to look for a character as the next parameter.
   //
   virtual Rc GetCharParmRc(char& c, CliThread& cli) const override;

   //  Overridden to look for a pointer as the next parameter.
   //
   virtual Rc GetPtrParmRc(void*& p, CliThread& cli) const override;

   //  Overridden to look for a string as the next parameter.
   //
   virtual Rc GetTextParmRc
      (id_t& i, std::string& s, CliThread& cli) const override;

   //  Overridden to look for an arbitrary string as the next parameter.
   //
   virtual Rc GetStringRc(std::string& s, CliThread& cli) const override;

   //  Overridden to look for a filename as the next parameter.
   //
   virtual Rc GetFileNameRc(std::string& s, CliThread& cli) const override;

   //  Overridden to look for an identifier as the next parameter.
   //
   virtual Rc GetIdentifierRc(std::string& s, CliThread& cli,
      const std::string& valid, const std::string& exclude) const override;

   //  Explains the command.  If VERBOSE is true, all of the command's
   //  parameters are also explained.  Returns 0.
   //
   word ExplainCommand(std::ostream& stream, bool verbose) const;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  HELP, COMM (the command's name), and SIZE (the maximum number
   //  of parameters that follow the command) are passed to CliText.
   //  Protected because this is class is virtual.
   //
   CliCommand(const char* comm, const char* help, size_t size = 32);

   //  This version of ProcessCommand allows a command to appear in more
   //  than one module.  The command's first parameter is a CliTextParm
   //  that registers "subcommand" strings at fixed indices.  In the base
   //  class, the basic version of ProcessCommand (above) parses the index
   //  and then invokes this version of ProcessCommand, passing the index
   //  as an argument.  A subclass overrides this version to implement its
   //  extensions to the base class, invoking the base class when INDEX is
   //  outside its range.  The default version generates a log and returns
   //  -1, and can be invoked to report an error if INDEX is out of bounds.
   //
   virtual word ProcessSubcommand(CliThread& cli, id_t index) const;
private:
   //  The function that actually implements the command.  It is invoked
   //  when the command is entered through the CLI.  If the command takes
   //  parameters, it calls a series of "Get" functions to obtain them.
   //  The command then performs its work.  Return values are command
   //  specific, but a zero or positive value generally denotes success,
   //  with negative values denoting errors or warnings.
   //
   virtual word ProcessCommand(CliThread& cli) const = 0;

   //  Overridden to stop looking for parameters if those below this
   //  command have been exhasted.
   //
   virtual bool Ascend() const override { return false; }

   //  Invoked if trying to obtain another parameter when the parse tree
   //  has been exhausted.  TYPE is the type of parameter that could not
   //  be obtained.
   //
   static Rc Exhausted(CliThread& cli, const std::string& type);
};
}
#endif
