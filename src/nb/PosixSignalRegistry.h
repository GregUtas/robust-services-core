//==============================================================================
//
//  PosixSignalRegistry.h
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
#ifndef POSIXSIGNALREGISTRY_H_INCLUDED
#define POSIXSIGNALREGISTRY_H_INCLUDED

#include "Immutable.h"
#include <string>
#include "NbTypes.h"
#include "Registry.h"
#include "SysTypes.h"

namespace NodeBase
{
   class PosixSignal;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for POSIX signals.
//
class PosixSignalRegistry : public Immutable
{
   friend class Singleton<PosixSignalRegistry>;
   friend class PosixSignal;
public:
   //  Deleted to prohibit copying.
   //
   PosixSignalRegistry(const PosixSignalRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   PosixSignalRegistry& operator=(const PosixSignalRegistry& that) = delete;

   //  Returns the signal identified by VALUE.
   //
   PosixSignal* Find(signal_t value) const;

   //  Returns the signal identified by NAME.
   //
   PosixSignal* Find(const std::string& name) const;

   //  Returns a string containing VALUE and the signal's name and explanation.
   //
   std::string strSignal(signal_t value) const;

   //  Returns the attributes for the signal identified by VALUE.
   //
   Flags Attrs(signal_t value) const;

   //  Returns the signal defined by NAME if it can be thrown on this platform.
   //  Returns SIGNIL if the signal cannot be thrown.
   //
   signal_t Value(const std::string& name) const;

   //  Returns the registry of signals.  Used for iteration.
   //
   const Registry<PosixSignal>& Signals() const { return signals_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to display each signal.
   //
   size_t Summarize(std::ostream& stream, uint32_t selector) const override;
private:
   //  Private because this is a singleton.
   //
   PosixSignalRegistry();

   //  Private because this is a singleton.
   //
   ~PosixSignalRegistry();

   //  Adds SIGNAL to the registry.
   //
   bool BindSignal(PosixSignal& signal);

   //  Removes SIGNAL from the registry.
   //
   void UnbindSignal(PosixSignal& signal);

   //  The global registry of POSIX signals.
   //
   Registry<PosixSignal> signals_;
};
}
#endif
