//==============================================================================
//
//  ToolRegistry.cpp
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
#include "ToolRegistry.h"
#include <cctype>
#include <ostream>
#include "Debug.h"
#include "Formatters.h"
#include "Tool.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const size_t ToolRegistry::MaxTools = 20;

//------------------------------------------------------------------------------

fn_name ToolRegistry_ctor = "ToolRegistry.ctor";

ToolRegistry::ToolRegistry()
{
   Debug::ft(ToolRegistry_ctor);

   tools_.Init(MaxTools + 1, Tool::CellDiff(), MemImm);
}

//------------------------------------------------------------------------------

fn_name ToolRegistry_dtor = "ToolRegistry.dtor";

ToolRegistry::~ToolRegistry()
{
   Debug::ft(ToolRegistry_dtor);
}

//------------------------------------------------------------------------------

fn_name ToolRegistry_BindTool = "ToolRegistry.BindTool";

bool ToolRegistry::BindTool(Tool& tool)
{
   Debug::ft(ToolRegistry_BindTool);

   //  Check that TOOL's CLI character is not already in use.
   //
   auto c = tool.CliChar();

   if(isprint(c))
   {
      for(auto t = tools_.First(); t != nullptr; tools_.Next(t))
      {
         if(t->CliChar() == c)
         {
            Debug::SwLog(ToolRegistry_BindTool, strClass(this), int(c));
            return false;
         }
      }
   }

   return tools_.Insert(tool);
}

//------------------------------------------------------------------------------

void ToolRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "tools : " << CRLF;
   tools_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name ToolRegistry_FindTool = "ToolRegistry.FindTool";

Tool* ToolRegistry::FindTool(char abbr) const
{
   Debug::ft(ToolRegistry_FindTool);

   if(!isprint(abbr)) return nullptr;

   for(auto t = tools_.First(); t != nullptr; tools_.Next(t))
   {
      if(t->CliChar() == abbr) return t;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

Tool* ToolRegistry::GetTool(FlagId id) const
{
   return tools_.At(id);
}

//------------------------------------------------------------------------------

fn_name ToolRegistry_ListTools = "ToolRegistry.ListTools";

string ToolRegistry::ListTools() const
{
   Debug::ft(ToolRegistry_ListTools);

   string tools;

   for(auto t = tools_.First(); t != nullptr; tools_.Next(t))
   {
      tools.push_back(t->CliChar());
   }

   return tools;
}

//------------------------------------------------------------------------------

void ToolRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name ToolRegistry_UnbindTool = "ToolRegistry.UnbindTool";

void ToolRegistry::UnbindTool(Tool& tool)
{
   Debug::ft(ToolRegistry_UnbindTool);

   tools_.Erase(tool);
}
}
