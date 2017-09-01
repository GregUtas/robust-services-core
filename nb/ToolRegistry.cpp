//==============================================================================
//
//  ToolRegistry.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "ToolRegistry.h"
#include <cctype>
#include <ostream>
#include <string>
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
            Debug::SwErr(ToolRegistry_BindTool, int(c), tool.Tid());
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

Tool* ToolRegistry::GetTool(FlagId fid) const
{
   return tools_.At(fid);
}

//------------------------------------------------------------------------------

void ToolRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name ToolRegistry_UnbindTool = "ToolRegistry.UnbindTool";

void ToolRegistry::UnbindTool(Tool& incr)
{
   Debug::ft(ToolRegistry_UnbindTool);

   tools_.Erase(incr);
}
}
