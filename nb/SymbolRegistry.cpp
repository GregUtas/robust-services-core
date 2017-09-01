//==============================================================================
//
//  SymbolRegistry.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "SymbolRegistry.h"
#include <cstring>
#include <ostream>
#include "Debug.h"
#include "Formatters.h"
#include "Symbol.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const size_t SymbolRegistry::MaxSymbols = 4000;

//------------------------------------------------------------------------------

fn_name SymbolRegistry_ctor = "SymbolRegistry.ctor";

SymbolRegistry::SymbolRegistry()
{
   Debug::ft(SymbolRegistry_ctor);

   symbolq_.Init(Symbol::LinkDiff());
}

//------------------------------------------------------------------------------

fn_name SymbolRegistry_dtor = "SymbolRegistry.dtor";

SymbolRegistry::~SymbolRegistry()
{
   Debug::ft(SymbolRegistry_dtor);

   symbolq_.Purge();
}

//------------------------------------------------------------------------------

fn_name SymbolRegistry_BindSymbol1 = "SymbolRegistry.BindSymbol(int)";

bool SymbolRegistry::BindSymbol(const string& name, word value, bool lock)
{
   Debug::ft(SymbolRegistry_BindSymbol1);

   auto sym = EnsureSymbol(name);
   if(sym == nullptr) return false;
   return sym->SetValue(strInt(value), lock);
}

//------------------------------------------------------------------------------

fn_name SymbolRegistry_BindSymbol2 = "SymbolRegistry.BindSymbol(string)";

bool SymbolRegistry::BindSymbol
   (const string& name, const string& value, bool lock)
{
   Debug::ft(SymbolRegistry_BindSymbol2);

   auto sym = EnsureSymbol(name);
   if(sym == nullptr) return false;
   return sym->SetValue(value, lock);
}

//------------------------------------------------------------------------------

void SymbolRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "symbolq : " << CRLF;
   symbolq_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name SymbolRegistry_EnsureSymbol = "SymbolRegistry.EnsureSymbol";

Symbol* SymbolRegistry::EnsureSymbol(const string& name)
{
   Debug::ft(SymbolRegistry_EnsureSymbol);

   auto sym = FindSymbol(name);
   if(sym != nullptr) return sym;

   if(symbolq_.Size() >= MaxSymbols) return nullptr;

   sym = new Symbol(name);
   if(sym == nullptr) return nullptr;

   //  Register symbols by name, in alphabetical order.
   //
   Symbol* prev = nullptr;

   for(auto next = symbolq_.First(); next != nullptr; symbolq_.Next(next))
   {
      if(name < next->Name()) break;

      if(name == next->Name())
      {
         Debug::SwErr(SymbolRegistry_EnsureSymbol, 0, 0);
         return next;
      }

      prev = next;
   }

   symbolq_.Insert(prev, *sym);
   return sym;
}

//------------------------------------------------------------------------------

fn_name SymbolRegistry_FindSymbol = "SymbolRegistry.FindSymbol";

Symbol* SymbolRegistry::FindSymbol(const string& name) const
{
   Debug::ft(SymbolRegistry_FindSymbol);

   for(auto s = symbolq_.First(); s != nullptr; symbolq_.Next(s))
   {
      if(strcmp(s->Name(), name.c_str()) == 0) return s;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void SymbolRegistry::Patch(sel_t selector, void* arguments)
{
   Dynamic::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name SymbolRegistry_RemoveSymbol = "SymbolRegistry.RemoveSymbol";

void SymbolRegistry::RemoveSymbol(Symbol& sym)
{
   Debug::ft(SymbolRegistry_RemoveSymbol);

   symbolq_.Exq(sym);
}
}
