//==============================================================================
//
//  Symbol.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef SYMBOL_H_INCLUDED
#define SYMBOL_H_INCLUDED

#include "Permanent.h"
#include <cstddef>
#include <string>
#include "Q1Link.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Base class for CLI symbols.  A symbol is used as a mnemonic to represent
//  a numeric value or string in CLI commands.  Symbols are defined through
//  SymbolRegistry::BindSymbol rather than by using this class directly.
//
class Symbol : public Permanent
{
public:
   //  Creates a symbol with NAME.
   //
   explicit Symbol(const std::string& name);

   //  Removes the symbol from SymbolRegistry.  Not subclassed.
   //
   ~Symbol();

   //  Deleted to prohibit copying.
   //
   Symbol(const Symbol& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   Symbol& operator=(const Symbol& that) = delete;

   //  Returns a string containing the characters that are valid in a
   //  symbol name.
   //
   static const std::string& ValidNameChars();

   //  Returns a string containing the characters that are invalid as
   //  the first character in a symbol name.
   //
   static const std::string& InvalidInitialChars();

   //  Returns the symbol's name.
   //
   c_string Name() const { return name_.c_str(); }

   //  Sets the symbol's value, locking it if LOCK is set.
   //  Returns false if the symbol is already locked.
   //
   bool SetValue(const std::string& value, bool lock);

   //  Returns the symbol's value.
   //
   c_string GetValue() const { return value_.c_str(); }

   //  Returns true if the value is locked.
   //
   bool IsLocked() const { return locked_; }

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  The symbol's name.
   //
   const std::string name_;

   //  The symbol's value.
   //
   std::string value_;

   //  Set if the value is locked.
   //
   bool locked_;

   //  The queue link for the symbol registry.
   //
   Q1Link link_;
};
}
#endif
