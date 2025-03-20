//==============================================================================
//
//  CliCommandSet.cpp
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
#include "CliCommandSet.h"
#include <sstream>
#include <string>
#include "CliBuffer.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "NbCliParms.h"
#include "Registry.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
CliCommandSet::CliCommandSet(c_string comm,
   c_string help, uint32_t size) : CliCommand(comm, help, size)
{
   Debug::ft("CliCommandSet.ctor");
}

//------------------------------------------------------------------------------

CliCommandSet::~CliCommandSet()
{
   Debug::ftnt("CliCommandSet.dtor");
}

//------------------------------------------------------------------------------

fn_name CliCommandSet_BindCommand = "CliCommandSet.BindCommand";

bool CliCommandSet::BindCommand(CliCommand& comm)
{
   Debug::ft(CliCommandSet_BindCommand);

   //  Generate a log and fail if
   //  o COMM has no name
   //  o another entry has the same name as COMM, which would make
   //    COMM inaccessible.
   //
   auto s = comm.Text();

   if(s[0] == NUL)
   {
      Debug::SwLog(CliCommandSet_BindCommand, "null name", 0);
      return false;
   }

   auto& commands = reinterpret_cast<Registry<CliCommand>& >(Parms());

   for(auto c = commands.First(); c != nullptr; commands.Next(c))
   {
      if(c->Text() == s)
      {
         Debug::SwLog(CliCommandSet_BindCommand, s, c->GetId());
         return false;
      }
   }

   return commands.Insert(comm);
}

//------------------------------------------------------------------------------

fn_name CliCommandSet_BindParm = "CliCommandSet.BindParm";

bool CliCommandSet::BindParm(CliParm& parm)
{
   Debug::ft(CliCommandSet_BindParm);

   //  This class only accepts other commands as parameters.
   //
   Debug::SwLog(CliCommandSet_BindParm, strClass(&parm), Parms().Size());
   return false;
}

//------------------------------------------------------------------------------

word CliCommandSet::ExplainCommand(ostream& stream, bool verbose) const
{
   Debug::ft("CliCommandSet.ExplainCommand");

   if(verbose)
   {
      //  This class optimizes out the CliTextParm that would otherwise hold
      //  the commands in the set, so include the symbols which indicate that
      //  one of our subcommands must follow our command name.
      //
      CliParm::Explain(stream, 0);

      stream << MandParmBegin << CRLF;

      const auto& parms = Parms();

      for(auto p = parms.First(); p != nullptr; parms.Next(p))
      {
         p->Explain(stream, 2);
      }

      stream << MandParmEnd << CRLF;
   }
   else
   {
      CliCommand::ExplainCommand(stream, verbose);
   }

   return 0;
}

//------------------------------------------------------------------------------

void CliCommandSet::Patch(sel_t selector, void* arguments)
{
   CliText::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

word CliCommandSet::ProcessCommand(CliThread& cli) const
{
   Debug::ft("CliCommandSet.ProcessCommand");

   string s;
   string tag;

   //  Save the current location in the input stream.  Get the
   //  next token, which must be the name of a command.
   //
   auto x = cli.ibuf->Pos();

   if(cli.ibuf->GetStr(tag, s) != Ok)
   {
      cli.ibuf->ErrorAtPos(cli, "Subcommand expected", x);
      return -1;
   }

   //  Commands are not tagged.
   //
   if(!tag.empty())
   {
      cli.ibuf->ErrorAtPos(cli, "Subcommands cannot be tagged", x);
      return -1;
   }

   //  Look for the command in our parameter registry, which contains
   //  only CliCommands.  If it is found, invoke it after updating the
   //  parser so that its parameters can be read.
   //
   const auto& commands =
      reinterpret_cast<const Registry<CliCommand>& >(Parms());

   for(auto c = commands.First(); c != nullptr; c = commands.Next(*c))
   {
      if(c->Text() == s)
      {
         return cli.InvokeSubcommand(*c);
      }
   }

   *cli.obuf << spaces(2) << NoCommandExpl << s << CRLF;
   return -1;
}
}
