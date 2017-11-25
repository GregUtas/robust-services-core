//==============================================================================
//
//  SymbolRegistry.h
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
#ifndef SYMBOLREGISTRY_H_INCLUDED
#define SYMBOLREGISTRY_H_INCLUDED

#include "Dynamic.h"
#include <cstddef>
#include <string>
#include "NbTypes.h"
#include "Q1Way.h"
#include "SysTypes.h"

namespace NodeBase
{
   class Symbol;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for symbols.
//
class SymbolRegistry : public Dynamic
{
   friend class Singleton< SymbolRegistry >;
public:
   //  Creates or finds the record for the symbol identified by NAME and
   //  sets its value to VALUE, locking it if LOCK is set.  Returns false
   //  if the symbol is locked to a different value.
   //
   bool BindSymbol
      (const std::string& name, word value, bool lock = true);
   bool BindSymbol
      (const std::string& name, const std::string& value, bool lock = true);

   //  Creates (or finds) the record for the symbol identified by NAME.
   //
   Symbol* EnsureSymbol(const std::string& name);

   //  Finds the record for the symbol identified by NAME.
   //
   Symbol* FindSymbol(const std::string& name) const;

   //  Removes SYM from the registry.  It must be explicitly deleted.
   //
   void RemoveSymbol(Symbol& sym);

   //  Returns the registry of symbols.  Used for iteration.
   //
   const Q1Way< Symbol >& Symbols() const { return symbolq_; }

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   SymbolRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~SymbolRegistry();

   //  Overridden to prohibit copying.
   //
   SymbolRegistry(const SymbolRegistry& that);
   void operator=(const SymbolRegistry& that);

   //> The maximum number of symbols allowed in symbolq_.
   //
   static const size_t MaxSymbols;

   //  The registry of symbols.
   //
   Q1Way< Symbol > symbolq_;
};
}
#endif
