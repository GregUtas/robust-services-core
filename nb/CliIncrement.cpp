//==============================================================================
//
//  CliIncrement.cpp
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
#include "CliIncrement.h"
#include <cstring>
#include <ostream>
#include "Algorithms.h"
#include "CliCommand.h"
#include "CliRegistry.h"
#include "Debug.h"
#include "Formatters.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CliIncrement_ctor = "CliIncrement.ctor";

CliIncrement::CliIncrement(c_string name, c_string help, uint32_t size) :
   name_(name),
   help_(help)
{
   Debug::ft(CliIncrement_ctor);

   Debug::Assert(name_ != nullptr);
   Debug::Assert(help_ != nullptr);

   commands_.Init(size, CliParm::CellDiff(), MemImmutable);
   Singleton< CliRegistry >::Instance()->BindIncrement(*this);
}

//------------------------------------------------------------------------------

fn_name CliIncrement_dtor = "CliIncrement.dtor";

CliIncrement::~CliIncrement()
{
   Debug::ftnt(CliIncrement_dtor);

   Debug::SwLog(CliIncrement_dtor, UnexpectedInvocation, 0);
   Singleton< CliRegistry >::Extant()->UnbindIncrement(*this);
}

//------------------------------------------------------------------------------

fn_name CliIncrement_BindCommand = "CliIncrement.BindCommand";

bool CliIncrement::BindCommand(CliCommand& comm)
{
   Debug::ft(CliIncrement_BindCommand);

   //  Generate a log and fail if
   //  o COMM has no name (a wildcard match), or
   //  o another entry has the same name as COMM, which would make
   //    COMM inaccessible.
   //
   auto s = comm.Text();

   if(s[0] == NUL)
   {
      Debug::SwLog(CliIncrement_BindCommand, "null name", 0);
      return false;
   }

   for(auto c = commands_.First(); c != nullptr; commands_.Next(c))
   {
      if(c->Text() == s)
      {
         Debug::SwLog(CliIncrement_BindCommand, s, c->GetId());
         return false;
      }
   }

   return commands_.Insert(comm);
}

//------------------------------------------------------------------------------

ptrdiff_t CliIncrement::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const CliIncrement* >(&local);
   return ptrdiff(&fake->iid_, fake);
}

//------------------------------------------------------------------------------

void CliIncrement::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "iid  : " << iid_.to_str() << CRLF;
   stream << prefix << "name : " << name_ << CRLF;
   stream << prefix << "help : " << help_ << CRLF;
   stream << prefix << "commands : " << CRLF;
   commands_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name CliIncrement_Enter = "CliIncrement.Enter";

void CliIncrement::Enter()
{
   Debug::ft(CliIncrement_Enter);

   //  This function is overridden by subclasses that require it.
}

//------------------------------------------------------------------------------

fn_name CliIncrement_Exit = "CliIncrement.Exit";

void CliIncrement::Exit()
{
   Debug::ft(CliIncrement_Exit);

   //  This function is overridden by subclasses that require it.
}

//------------------------------------------------------------------------------

fn_name CliIncrement_Explain = "CliIncrement.Explain";

word CliIncrement::Explain(ostream& stream, int level) const
{
   Debug::ft(CliIncrement_Explain);

   switch(level)
   {
   case 0:
      stream << spaces(CliCommand::CommandWidth - strlen(name_)) << name_;
      stream << CliParm::ParmExplPrefix << help_ << CRLF;
      break;

   case 1:
      for(auto c = commands_.First(); c != nullptr; commands_.Next(c))
      {
         c->ExplainCommand(stream, false);
      }
      break;

   default:
      for(auto c = commands_.First(); c != nullptr; commands_.Next(c))
      {
         c->ExplainCommand(stream, true);
         stream << CRLF;
      }
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name CliIncrement_FindCommand = "CliIncrement.FindCommand";

CliCommand* CliIncrement::FindCommand(const string& comm) const
{
   Debug::ft(CliIncrement_FindCommand);

   //  Search the increment's commands for one whose name is COMM.
   //
   for(auto c = commands_.First(); c != nullptr; commands_.Next(c))
   {
      if(c->Text() == comm) return c;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void CliIncrement::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}
}
