//==============================================================================
//
//  CliRegistry.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CliRegistry.h"
#include <memory>
#include <ostream>
#include "CfgParmRegistry.h"
#include "CfgStrParm.h"
#include "CliIncrement.h"
#include "Debug.h"
#include "Formatters.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const size_t CliRegistry::MaxIncrements = 30;
string CliRegistry::ConsoleFileName_ = "console";

//------------------------------------------------------------------------------

fn_name CliRegistry_ctor = "CliRegistry.ctor";

CliRegistry::CliRegistry()
{
   Debug::ft(CliRegistry_ctor);

   increments_.Init(MaxIncrements + 1, CliIncrement::CellDiff(), MemProt);

   consoleFileName_.reset(new CfgFileTimeParm("ConsoleFileName",
      "console", &ConsoleFileName_, "name for console transcript files"));
   Singleton< CfgParmRegistry >::Instance()->BindParm(*consoleFileName_);
}

//------------------------------------------------------------------------------

fn_name CliRegistry_dtor = "CliRegistry.dtor";

CliRegistry::~CliRegistry()
{
   Debug::ft(CliRegistry_dtor);
}

//------------------------------------------------------------------------------

fn_name CliRegistry_BindIncrement = "CliRegistry.BindIncrement";

bool CliRegistry::BindIncrement(CliIncrement& incr)
{
   Debug::ft(CliRegistry_BindIncrement);

   return increments_.Insert(incr);
}

//------------------------------------------------------------------------------

void CliRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix
      << "ConsoleFileName : " << ConsoleFileName() << CRLF;
   stream << prefix
      << "consoleFileName : " << strObj(consoleFileName_.get()) << CRLF;
   stream << prefix << "increments : " << CRLF;
   increments_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name CliRegistry_FindIncrement = "CliRegistry.FindIncrement";

CliIncrement* CliRegistry::FindIncrement(const string& name) const
{
   Debug::ft(CliRegistry_FindIncrement);

   //  Look for the increment that is known by NAME.
   //
   for(auto i = increments_.First(); i != nullptr; increments_.Next(i))
   {
      if(i->Name() == name) return i;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name CliRegistry_ListIncrements = "CliRegistry.ListIncrements";

void CliRegistry::ListIncrements(ostream& stream) const
{
   Debug::ft(CliRegistry_ListIncrements);

   //  Output a brief description of each increment.
   //
   for(auto i = increments_.First(); i != nullptr; increments_.Next(i))
   {
      i->Explain(stream, 0);
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

void CliRegistry::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CliRegistry_UnbindIncrement = "CliRegistry.UnbindIncrement";

void CliRegistry::UnbindIncrement(CliIncrement& incr)
{
   Debug::ft(CliRegistry_UnbindIncrement);

   increments_.Erase(incr);
}
}
