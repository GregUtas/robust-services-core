//==============================================================================
//
//  CliCommandSet.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CliCommandSet.h"
#include <cstring>
#include <sstream>
#include <string>
#include "CliBuffer.h"
#include "CliCookie.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "NbCliParms.h"
#include "Registry.h"
#include "SysTypes.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CliCommandSet_ctor = "CliCommandSet.ctor";

CliCommandSet::CliCommandSet(const char* comm, const char* help, size_t size) :
   CliCommand(comm, help, size)
{
   Debug::ft(CliCommandSet_ctor);
}

//------------------------------------------------------------------------------

fn_name CliCommandSet_dtor = "CliCommandSet.dtor";

CliCommandSet::~CliCommandSet()
{
   Debug::ft(CliCommandSet_dtor);
}

//------------------------------------------------------------------------------

fn_name CliCommandSet_BindCommand = "CliCommandSet.BindCommand";

bool CliCommandSet::BindCommand(CliCommand& comm)
{
   Debug::ft(CliCommandSet_BindCommand);

   //  Generate a log and fail if
   //  o COMM has no name (a wildcard match), or
   //  o another entry has the same name as COMM, which would make
   //    COMM inaccessible.
   //
   auto s = comm.Text();

   if(strlen(s) == 0)
   {
      Debug::SwErr(CliCommandSet_BindCommand, 0, 0);
      return false;
   }

   auto& commands = reinterpret_cast< Registry< CliCommand >& >(Parms());

   for(auto c = commands.First(); c != nullptr; commands.Next(c))
   {
      if(c->Text() == s)
      {
         Debug::SwErr(CliCommandSet_BindCommand, c->GetId(), 1);
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
   Debug::SwErr(CliCommandSet_BindParm, Parms().Size(), 0);
   return false;
}

//------------------------------------------------------------------------------

fn_name CliCommandSet_DescendTo = "CliCommandSet.DescendTo";

void CliCommandSet::DescendTo(CliCookie& cookie, size_t index)
{
   Debug::ft(CliCommandSet_DescendTo);

   cookie.Descend(index);
}

//------------------------------------------------------------------------------

void CliCommandSet::Patch(sel_t selector, void* arguments)
{
   CliText::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CliCommandSet_ProcessCommand = "CliCommandSet.ProcessCommand";

word CliCommandSet::ProcessCommand(CliThread& cli) const
{
   Debug::ft(CliCommandSet_ProcessCommand);

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
   auto& commands = reinterpret_cast< const Registry< CliCommand >& >(Parms());

   for(auto c = commands.First(); c != nullptr; c = commands.Next(*c))
   {
      if(c->Text() == s)
      {
         DescendTo(cli.Cookie(), c->GetId());
         return cli.InvokeSubcommand(*c);
      }
   }

   *cli.obuf << spaces(2) << NoCommandExpl << s << CRLF;
   return -1;
}
}
