//==============================================================================
//
//  CliCommand.cpp
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
#include "CliCommand.h"
#include <cstring>
#include <ostream>
#include "CliBuffer.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const int CliCommand::CommandWidth = 12;
const char CliCommand::CommandSeparator = '.';

//------------------------------------------------------------------------------

fn_name CliCommand_ctor = "CliCommand.ctor";

CliCommand::CliCommand(const char* comm, const char* help, size_t size) :
   CliText(help, comm, false, size)
{
   Debug::ft(CliCommand_ctor);

   if((comm != nullptr) && (strlen(comm) > CommandWidth))
   {
      Debug::SwErr(CliCommand_ctor, strlen(comm), 0);
   }
}

//------------------------------------------------------------------------------

fn_name CliCommand_dtor = "CliCommand.dtor";

CliCommand::~CliCommand()
{
   Debug::ft(CliCommand_dtor);
}

//------------------------------------------------------------------------------

fn_name CliCommand_Exhausted = "CliCommand.Exhausted";

CliParm::Rc CliCommand::Exhausted(const CliThread& cli, const string& type)
{
   Debug::ft(CliCommand_Exhausted);

   auto s = "Internal error: parameters exhausted before looking for " + type;
   cli.ibuf->ErrorAtPos(cli, s);
   return Error;
}

//------------------------------------------------------------------------------

fn_name CliCommand_ExplainCommand = "CliCommand.ExplainCommand";

word CliCommand::ExplainCommand(ostream& stream, bool verbose) const
{
   Debug::ft(CliCommand_ExplainCommand);

   if(verbose)
   {
      //  Provide full help by invoking Explain (on our base class) to
      //  display the purpose of the command and each of its parameters.
      //
      Explain(stream, 0);
   }
   else
   {
      //  We are listing all of the commands in the increment.  Display only
      //  each command's name and its purpose.
      //
      auto indent = CommandWidth - strlen(Text());
      if(indent > 0) stream << spaces(indent);
      stream << Text() << ParmExplPrefix;
      stream << Help() << CRLF;
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name CliCommand_GetBoolParmRc = "CliCommand.GetBoolParmRc";

CliParm::Rc CliCommand::GetBoolParmRc(bool& b, CliThread& cli) const
{
   Debug::ft(CliCommand_GetBoolParmRc);

   //  Return the next parameter, which should be a boolean.
   //
   auto parm = AccessParm(cli.Cookie(), 0);
   if(parm == nullptr) return Exhausted(cli, "boolean");
   return parm->GetBoolParmRc(b, cli);
}

//------------------------------------------------------------------------------

fn_name CliCommand_GetCharParmRc = "CliCommand.GetCharParmRc";

CliParm::Rc CliCommand::GetCharParmRc(char& c, CliThread& cli) const
{
   Debug::ft(CliCommand_GetCharParmRc);

   //  Return the next parameter, which should be a character.
   //
   auto parm = AccessParm(cli.Cookie(), 0);
   if(parm == nullptr) return Exhausted(cli, "character");
   return parm->GetCharParmRc(c, cli);
}

//------------------------------------------------------------------------------

fn_name CliCommand_GetFileNameRc = "CliCommand.GetFileNameRc";

CliCommand::Rc CliCommand::GetFileNameRc(string& s, CliThread& cli) const
{
   Debug::ft(CliCommand_GetFileNameRc);

   //  Return the next parameter, which should be a filename.
   //
   auto parm = AccessParm(cli.Cookie(), 0);
   if(parm == nullptr) return Exhausted(cli, "filename");
   return parm->GetFileNameRc(s, cli);
}

//------------------------------------------------------------------------------

fn_name CliCommand_GetIdentifierRc = "CliCommand.GetIdentifierRc";

CliParm::Rc CliCommand::GetIdentifierRc(string& s, CliThread& cli,
   const string& valid, const string& exclude) const
{
   Debug::ft(CliCommand_GetIdentifierRc);

   //  Return the next parameter, which should be an identifier.
   //
   auto parm = AccessParm(cli.Cookie(), 0);
   if(parm == nullptr) return Exhausted(cli, "identifier");
   return parm->GetIdentifierRc(s, cli, valid, exclude);
}

//------------------------------------------------------------------------------

fn_name CliCommand_GetIntParmRc = "CliCommand.GetIntParmRc";

CliParm::Rc CliCommand::GetIntParmRc(word& n, CliThread& cli) const
{
   Debug::ft(CliCommand_GetIntParmRc);

   //  Return the next parameter, which should be an integer.
   //
   auto parm = AccessParm(cli.Cookie(), 0);
   if(parm == nullptr) return Exhausted(cli, "integer");
   return parm->GetIntParmRc(n, cli);
}

//------------------------------------------------------------------------------

fn_name CliCommand_GetPtrParmRc = "CliCommand.GetPtrParmRc";

CliParm::Rc CliCommand::GetPtrParmRc(void*& p, CliThread& cli) const
{
   Debug::ft(CliCommand_GetPtrParmRc);

   //  Return the next parameter, which should be a pointer.
   //
   auto parm = AccessParm(cli.Cookie(), 0);
   if(parm == nullptr) return Exhausted(cli, "pointer");
   return parm->GetPtrParmRc(p, cli);
}

//------------------------------------------------------------------------------

fn_name CliCommand_GetStringRc = "CliCommand.GetStringRc";

CliParm::Rc CliCommand::GetStringRc(string& s, CliThread& cli) const
{
   Debug::ft(CliCommand_GetStringRc);

   //  Return the next parameter, which can be any string.
   //
   auto parm = AccessParm(cli.Cookie(), 0);
   if(parm == nullptr) return Exhausted(cli, "string");
   return parm->GetStringRc(s, cli);
}

//------------------------------------------------------------------------------

fn_name CliCommand_GetTextParmRc = "CliCommand.GetTextParmRc";

CliParm::Rc CliCommand::GetTextParmRc(id_t& i, string& s, CliThread& cli) const
{
   Debug::ft(CliCommand_GetTextParmRc);

   //  Return the next parameter, which should be a string in a specified list.
   //
   auto parm = AccessParm(cli.Cookie(), 0);
   if(parm == nullptr) return Exhausted(cli, "text");
   return parm->GetTextParmRc(i, s, cli);
}

//------------------------------------------------------------------------------

void CliCommand::Patch(sel_t selector, void* arguments)
{
   CliText::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CliCommand_ProcessCommand = "CliCommand.ProcessCommand";

word CliCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(CliCommand_ProcessCommand);

   //  This is a pure virtual function.
   //
   Debug::SwErr(CliCommand_ProcessCommand, 0, 0, ErrorLog);
   return -1;
}

//------------------------------------------------------------------------------

fn_name CliCommand_ProcessSubcommand = "CliCommand.ProcessSubcommand";

word CliCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft(CliCommand_ProcessSubcommand);

   //  This can be invoked to generate a log.
   //
   Debug::SwErr(CliCommand_ProcessSubcommand, index, 0);
   return -1;
}
}
