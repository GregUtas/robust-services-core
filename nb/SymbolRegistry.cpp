//==============================================================================
//
//  SymbolRegistry.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "SymbolRegistry.h"
#include <cstddef>
#include <ostream>
#include "Debug.h"
#include "Formatters.h"
#include "Symbol.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//> The maximum number of symbols allowed in symbolq_.
//
constexpr size_t MaxSymbols = 4000;

//------------------------------------------------------------------------------

SymbolRegistry::SymbolRegistry()
{
   Debug::ft("SymbolRegistry.ctor");

   symbolq_.Init(Symbol::LinkDiff());
}

//------------------------------------------------------------------------------

fn_name SymbolRegistry_dtor = "SymbolRegistry.dtor";

SymbolRegistry::~SymbolRegistry()
{
   Debug::ftnt(SymbolRegistry_dtor);

   Debug::SwLog(SymbolRegistry_dtor, UnexpectedInvocation, 0);
   symbolq_.Purge();
}

//------------------------------------------------------------------------------

bool SymbolRegistry::BindSymbol(const string& name, word value, bool lock)
{
   Debug::ft("SymbolRegistry.BindSymbol(int)");

   auto sym = EnsureSymbol(name);
   if(sym == nullptr) return false;
   return sym->SetValue(std::to_string(value), lock);
}

//------------------------------------------------------------------------------

bool SymbolRegistry::BindSymbol
   (const string& name, const string& value, bool lock)
{
   Debug::ft("SymbolRegistry.BindSymbol(string)");

   auto sym = EnsureSymbol(name);
   if(sym == nullptr) return false;
   return sym->SetValue(value, lock);
}

//------------------------------------------------------------------------------

void SymbolRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   stream << prefix << "symbolq : " << CRLF;
   symbolq_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

Symbol* SymbolRegistry::EnsureSymbol(const string& name)
{
   Debug::ft("SymbolRegistry.EnsureSymbol");

   auto sym = FindSymbol(name);
   if(sym != nullptr) return sym;

   if(symbolq_.Size() >= MaxSymbols) return nullptr;

   sym = new Symbol(name);

   //  Register symbols by name, in alphabetical order.
   //
   Symbol* prev = nullptr;

   for(auto next = symbolq_.First(); next != nullptr; symbolq_.Next(next))
   {
      if(name < next->Name()) break;
      prev = next;
   }

   symbolq_.Insert(prev, *sym);
   return sym;
}

//------------------------------------------------------------------------------

Symbol* SymbolRegistry::FindSymbol(const string& name) const
{
   Debug::ft("SymbolRegistry.FindSymbol");

   for(auto s = symbolq_.First(); s != nullptr; symbolq_.Next(s))
   {
      if(name == s->Name()) return s;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void SymbolRegistry::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void SymbolRegistry::RemoveSymbol(Symbol& sym)
{
   Debug::ft("SymbolRegistry.RemoveSymbol");

   symbolq_.Exq(sym);
}
}
