//==============================================================================
//
//  CxxRoot.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CXXROOT_H_INCLUDED
#define CXXROOT_H_INCLUDED

#include "Temporary.h"
#include <iosfwd>
#include "CxxFwd.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
class CxxRoot : public Temporary
{
   friend class Singleton< CxxRoot >;
   friend NamespacePtr::deleter_type;
public:
   //  Returns the global namespace.
   //
   Namespace* GlobalNamespace() const { return gns_.get(); }

   //  Returns the corresponding built-in type.
   //
   Terminal* AutoTerm() const { return auto_.get(); }
   Terminal* BoolTerm() const { return bool_.get(); }
   Terminal* CharTerm() const { return char_.get(); }
   Terminal* DoubleTerm() const { return double_.get(); }
   Terminal* FloatTerm() const { return float_.get(); }
   Terminal* IntTerm() const { return int_.get(); }
   Terminal* LongTerm() const { return long_.get(); }
   Terminal* LongDoubleTerm() const { return long_double_.get(); }
   Terminal* LongLongTerm() const { return long_.get(); }
   Terminal* NullptrTerm() const { return nullptr_.get(); }
   Terminal* NullptrtTerm() const { return nullptr_t_.get(); }
   Terminal* ShortTerm() const { return short_.get(); }
   Terminal* uCharTerm() const { return uchar_.get(); }
   Terminal* uIntTerm() const { return uint_.get(); }
   Terminal* uLongTerm() const { return ulong_.get(); }
   Terminal* uLongLongTerm() const { return ulong_long_.get(); }
   Terminal* uShortTerm() const { return ushort_.get(); }
   Terminal* VoidTerm() const { return void_.get(); }

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
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for restarts.
   //
   virtual void Shutdown(RestartLevel level) override;

   //  Overridden for restarts.
   //
   virtual void Startup(RestartLevel level) override;
private:
   //  Private because this singleton is not subclassed.
   //
   CxxRoot();

   //  Private because this singleton is not subclassed.
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
   MacroPtrVector macros_;
};
}
#endif