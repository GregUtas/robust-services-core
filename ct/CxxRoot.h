//==============================================================================
//
//  CxxRoot.h
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
#ifndef CXXROOT_H_INCLUDED
#define CXXROOT_H_INCLUDED

#include "Base.h"
#include <iosfwd>
#include "CxxArea.h"
#include "CxxFwd.h"
#include "CxxScoped.h"
#include "NbTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
class CxxRoot : public NodeBase::Base
{
   friend class NodeBase::Singleton< CxxRoot >;
   friend NamespacePtr::deleter_type;
public:
   //  Deleted to prohibit copying.
   //
   CxxRoot(const CxxRoot& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   CxxRoot& operator=(const CxxRoot& that) = delete;

   //  Returns the global namespace.
   //
   Namespace* GlobalNamespace() const { return gns_.get(); }

   //  Returns the corresponding built-in type.
   //
   Terminal* AutoTerm() const { return auto_.get(); }
   Terminal* BoolTerm() const { return bool_.get(); }
   Terminal* CharTerm() const { return char_.get(); }
   Terminal* Char16Term() const { return char16_.get(); }
   Terminal* Char32Term() const { return char32_.get(); }
   Terminal* DoubleTerm() const { return double_.get(); }
   Terminal* FloatTerm() const { return float_.get(); }
   Terminal* IntTerm() const { return int_.get(); }
   Terminal* LongTerm() const { return long_.get(); }
   Terminal* LongDoubleTerm() const { return long_double_.get(); }
   Terminal* LongLongTerm() const { return long_long_.get(); }
   Terminal* NullptrTerm() const { return nullptr_.get(); }
   Terminal* NullptrtTerm() const { return nullptr_t_.get(); }
   Terminal* ShortTerm() const { return short_.get(); }
   Terminal* uCharTerm() const { return uchar_.get(); }
   Terminal* uIntTerm() const { return uint_.get(); }
   Terminal* uLongTerm() const { return ulong_.get(); }
   Terminal* uLongLongTerm() const { return ulong_long_.get(); }
   Terminal* uShortTerm() const { return ushort_.get(); }
   Terminal* VoidTerm() const { return void_.get(); }
   Terminal* wCharTerm() const { return wchar_.get(); }

   //  Registers a macro.
   //
   bool AddMacro(MacroPtr& macro);

   //  Creates #define symbols at the beginning of a compile.
   //
   void DefineSymbols(std::istream& stream);

   //  Shrinks containers.
   //
   void Shrink() const;

   //  Overridden to display macros.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for restarts.
   //
   void Shutdown(NodeBase::RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Startup(NodeBase::RestartLevel level) override;
private:
   //  Private because this is a singleton.
   //
   CxxRoot();

   //  Private because this is a singleton.
   //
   ~CxxRoot();

   //  The contents of the global namespace.  Everything lives below this.
   //
   NamespacePtr gns_;

   //  Terminals created during startup.
   //
   TerminalPtr auto_;
   TerminalPtr bool_;
   TerminalPtr char_;
   TerminalPtr char16_;
   TerminalPtr char32_;
   TerminalPtr double_;
   TerminalPtr float_;
   TerminalPtr int_;
   TerminalPtr long_;
   TerminalPtr long_double_;
   TerminalPtr long_long_;
   TerminalPtr nullptr_;
   TerminalPtr nullptr_t_;
   TerminalPtr short_;
   TerminalPtr uchar_;
   TerminalPtr uint_;
   TerminalPtr ulong_;
   TerminalPtr ulong_long_;
   TerminalPtr ushort_;
   TerminalPtr void_;
   TerminalPtr wchar_;
   MacroPtrVector macros_;
};
}
#endif
