//==============================================================================
//
//  CliStack.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CliStack.h"
#include <bitset>
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

CliStack::CliStack() : top_(0)
{
   Debug::ft(CliStack_ctor);

   increments_[0] = nullptr;
}

//------------------------------------------------------------------------------

fn_name CliStack_dtor = "CliStack.dtor";

CliStack::~CliStack()
{
   Debug::ft(CliStack_dtor);

   //  Exit all active increments.
   //
   for(auto i = top_; i >= 0; --i)
   {
      increments_[i]->Exit();
   }
}

//------------------------------------------------------------------------------

void CliStack::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Temporary::Display(stream, prefix, options);

   stream << prefix << "top : " << top_ << CRLF;

   auto lead1 = prefix + spaces(2);
   auto lead2 = prefix + spaces(4);

   stream << prefix << "increments : " << CRLF;

   for(auto i = top_; i >= 0; --i)
   {
      stream << lead1 << strIndex(i);

      if(options.test(DispVerbose))
      {
         stream << CRLF;
         increments_[i]->Display(stream, lead2, options);
      }
      else
      {
         stream << strObj(increments_[i]) << CRLF;
      }
   }
}

//------------------------------------------------------------------------------

fn_name CliStack_FindCommand = "CliStack.FindCommand";

const CliCommand* CliStack::FindCommand(const string& text) const
{
   Debug::ft(CliStack_FindCommand);

   const CliIncrement* incr;
   return FindCommand(text, incr);
}

//------------------------------------------------------------------------------

const CliCommand* CliStack::FindCommand
   (const string& text, const CliIncrement*& incr) const
{
   Debug::ft(CliStack_FindCommand);

   CliCommand* comm;

   //  Search the active increments for one that recognizes TEXT as
   //  a command.  If more than one increment has TEXT as a command,
   //  the most recently entered increment gets to handle it.
   //
   for(auto i = top_; i >= 0; --i)
   {
      comm = increments_[i]->FindCommand(text);

      if(comm != nullptr)
      {
         incr = increments_[i];
         return comm;
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
   for(auto i = top_; i >= 0; --i)
   {
      if(increments_[i]->Name() == name) return increments_[i];
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
   if(top_ > 0)
   {
      increments_[top_--]->Exit();
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name CliStack_Push = "CliStack.Push";

bool CliStack::Push(CliIncrement& incr)
{
   Debug::ft(CliStack_Push);

   //  If there is space on the stack, add INCR.
   //
   if(top_ < MaxIncrDepth)
   {
      increments_[++top_] = &incr;
      incr.Enter();
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name CliStack_SetRoot = "CliStack.SetRoot";

void CliStack::SetRoot(CliIncrement& root)
{
   Debug::ft(CliStack_SetRoot);

   //  If the stack is empty, add ROOT (the NodeBase increment) as the
   //  first increment.
   //
   if(increments_[0] == nullptr)
   {
      increments_[0] = &root;
      root.Enter();
   }
}

//------------------------------------------------------------------------------

fn_name CliStack_Top = "CliStack.Top";

CliIncrement* CliStack::Top() const
{
   Debug::ft(CliStack_Top);

   return increments_[top_];
}
}
