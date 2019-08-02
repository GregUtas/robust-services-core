//==============================================================================
//
//  Symbol.h
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
#ifndef SYMBOL_H_INCLUDED
#define SYMBOL_H_INCLUDED

#include "Dynamic.h"
#include <cstddef>
#include <string>
#include "NbTypes.h"
#include "Q1Link.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Base class for symbols.  A symbol is used as a mnemonic to
//  represent a numeric value in CLI commands.
//
class Symbol : public Dynamic
{
public:
   //  Creates a symbol with NAME.
   //
   explicit Symbol(const std::string& name);

   //  Removes the symbol from SymbolRegistry.  Not subclassed.
   //
   ~Symbol();

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
   const char* Name() const { return name_.c_str(); }

   //  Sets the symbol's value, locking it if LOCK is set.
   //  Returns false if the symbol is already locked.
   //
   bool SetValue(const std::string& value, bool lock);

   //  Returns the symbol's value.
   //
   const char* GetValue() const { return value_.c_str(); }

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
   //  Deleted to prohibit copying.
   //
   Symbol(const Symbol& that) = delete;
   Symbol& operator=(const Symbol& that) = delete;

   //  The symbol's name.
   //
   const DynString name_;

   //  The symbol's value.
   //
   DynString value_;

   //  Set if the value is locked.
   //
   bool locked_;

   //  The queue link for the symbol registry.
   //
   Q1Link link_;
};
}
#endif
