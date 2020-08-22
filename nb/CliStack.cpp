//==============================================================================
//
//  CliStack.cpp
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
#include "CliStack.h"
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include "CliIncrement.h"
#include "Debug.h"
#include "Formatters.h"
#include "NbTypes.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CliStack_ctor = "CliStack.ctor";

CliStack::CliStack()
{
   Debug::ft(CliStack_ctor);
}

//------------------------------------------------------------------------------

fn_name CliStack_dtor = "CliStack.dtor";

CliStack::~CliStack()
{
   Debug::ftnt(CliStack_dtor);

   //  Exit all active increments.
   //
   for(size_t i = increments_.size() - 1; i != SIZE_MAX; --i)
   {
      increments_.at(i)->Exit();
   }
}

//------------------------------------------------------------------------------

void CliStack::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Temporary::Display(stream, prefix, options);

   auto lead1 = prefix + spaces(2);
   auto lead2 = prefix + spaces(4);

   stream << prefix << "increments : " << CRLF;

   for(size_t i = increments_.size() - 1; i != SIZE_MAX; --i)
   {
      stream << lead1 << strIndex(i);

      if(options.test(DispVerbose))
      {
         stream << CRLF;
         increments_.at(i)->Display(stream, lead2, options);
      }
      else
      {
         stream << strObj(increments_.at(i)) << CRLF;
      }
   }
}

//------------------------------------------------------------------------------

fn_name CliStack_FindCommand = "CliStack.FindCommand";

const CliCommand* CliStack::FindCommand(const string& comm) const
{
   Debug::ft(CliStack_FindCommand);

   const CliIncrement* incr;
   return FindCommand(comm, incr);
}

//------------------------------------------------------------------------------

const CliCommand* CliStack::FindCommand
   (const string& comm, const CliIncrement*& incr) const
{
   Debug::ft(CliStack_FindCommand);

   //  Search the active increments for one that recognizes TEXT as
   //  a command.  If more than one increment has TEXT as a command,
   //  the most recently entered increment gets to handle it.
   //
   for(size_t i = increments_.size() - 1; i != SIZE_MAX; --i)
   {
      auto c = increments_.at(i)->FindCommand(comm);

      if(c != nullptr)
      {
         incr = increments_.at(i);
         return c;
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name CliStack_FindIncrement = "CliStack.FindIncrement";

CliIncrement* CliStack::FindIncrement(const string& name) const
{
   Debug::ft(CliStack_FindIncrement);

   //  Search the active increments for the one that is known by NAME.
   //
   for(size_t i = increments_.size() - 1; i != SIZE_MAX; --i)
   {
      if(increments_.at(i)->Name() == name) return increments_.at(i);
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void CliStack::Patch(sel_t selector, void* arguments)
{
   Temporary::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CliStack_Pop = "CliStack.Pop";

bool CliStack::Pop()
{
   Debug::ft(CliStack_Pop);

   //  Exit the increment on top of the stack, but always keep the NodeBase
   //  increment.
   //
   if(increments_.size() > 1)
   {
      increments_.back()->Exit();
      increments_.pop_back();
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name CliStack_Push = "CliStack.Push";

void CliStack::Push(CliIncrement& incr)
{
   Debug::ft(CliStack_Push);

   increments_.push_back(&incr);
   incr.Enter();
}

//------------------------------------------------------------------------------

fn_name CliStack_SetRoot = "CliStack.SetRoot";

void CliStack::SetRoot(CliIncrement& root)
{
   Debug::ft(CliStack_SetRoot);

   //  If the stack is empty, add ROOT (the NodeBase increment) as the
   //  first increment.
   //
   if(increments_.empty())
   {
      increments_.push_back(&root);
      root.Enter();
   }
}

//------------------------------------------------------------------------------

fn_name CliStack_Top = "CliStack.Top";

CliIncrement* CliStack::Top() const
{
   Debug::ft(CliStack_Top);

   return (increments_.empty() ? nullptr : increments_.back());
}
}
