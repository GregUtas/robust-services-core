//==============================================================================
//
//  Symbol.cpp
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
#include "Symbol.h"
#include <ostream>
#include "Algorithms.h"
#include "Debug.h"
#include "Singleton.h"
#include "SymbolRegistry.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//e Enhancements for symbols:
//  o During startup, read a file containing symbol names, similar to how
//    configuration parameters are handled.
//  o Formally support hierarchical names using '.' as a delimiter:
//    fixed_string SymbolMandNameExpl = "symbol's name ('.' is delimiter)";
//  o Store symbols in a tree whose interior nodes group symbols that share
//    the same hierarchical prefix.
//  o Support '*' as a wildcard in >symbols list:
//    fixed_string SymbolOptNameExpl =
//      "symbol's name (lists all if blank; '*' is wildcard)";
//    SymbolOptName::SymbolOptName()
//       : CliTextParm(&SymbolOptNameExpl) { }
//  o Support >symbols set <name> <expr>.  If the result of expr can change,
//    save it as a string and use Execute to evaluate it each time.
//
fn_name Symbol_ctor = "Symbol.ctor";

Symbol::Symbol(const string& name) :
   name_(name.c_str()),
   locked_(false)
{
   Debug::ft(Symbol_ctor);
}

//------------------------------------------------------------------------------

fn_name Symbol_dtor = "Symbol.dtor";

Symbol::~Symbol()
{
   Debug::ft(Symbol_dtor);

   Singleton< SymbolRegistry >::Instance()->RemoveSymbol(*this);
}

//------------------------------------------------------------------------------

void Symbol::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "name   : " << name_ << CRLF;
   stream << prefix << "value  : " << value_ << CRLF;
   stream << prefix << "locked : " << locked_ << CRLF;
}

//------------------------------------------------------------------------------

const string& Symbol::InvalidInitialChars()
{
   //> Characters that are invalid as the start of a symbol name.
   //
   static const string NonInitialChars("0123456789.");

   return NonInitialChars;
}

//------------------------------------------------------------------------------

ptrdiff_t Symbol::LinkDiff()
{
   int local;
   auto fake = reinterpret_cast< const Symbol* >(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

void Symbol::Patch(sel_t selector, void* arguments)
{
   Dynamic::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Symbol_SetValue= "Symbol.SetValue";

bool Symbol::SetValue(const string& value, bool lock)
{
   Debug::ft(Symbol_SetValue);

   if(locked_) return false;

   value_ = value.c_str();
   locked_ = lock;
   return true;
}

//------------------------------------------------------------------------------

const string& Symbol::ValidNameChars()
{
   //> Valid characters in a symbol name.
   //
   static const string NameChars
      ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.");

   return NameChars;
}
}
