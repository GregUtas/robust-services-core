//==============================================================================
//
//  PosixSignal.h
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
#ifndef POSIXSIGNAL_H_INCLUDED
#define POSIXSIGNAL_H_INCLUDED

#include "Protected.h"
#include <cstddef>
#include <cstdint>
#include "RegCell.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Base class for a POSIX signal.
//
class PosixSignal : public Protected
{
   friend class Registry< PosixSignal >;
public:
   //> Highest valid signal identifier.
   //
   static const signal_t MaxId = UINT8_MAX;

   //  Signal attributes.
   //
   enum Attribute
   {
      Native,       // supported by platform
      Break,        // interrupt received on unknown thread
      NoRecover,    // blocks invocation of Thread::Recover
      Interrupt,    // interrupts target thread
      Delayed,      // not received until scheduled out
      Exit,         // causes thread to exit
      Final,        // InitThread will not recreate thread
      NoLog,        // no log when raised for another thread
      NoError,      // no log from trap handler
      Attribute_N   // number of attributes
   };

   //  Returns the signal's value on this platform.
   //
   signal_t Value() const { return value_; }

   //  Returns the signal's name.
   //
   const char* Name() const { return name_; }

   //  Returns the signal's explanation.
   //
   const char* Expl() const { return expl_; }

   //  Returns the signal's severity.  A severity of zero indicates that
   //  the signal cannot be raised for another thread.  If a thread has a
   //  pending signal, a signal of greater severity replaces it.
   //
   uint8_t Severity() const { return severity_; }

   //  Returns the signal's attributes.
   //
   const Flags& Attrs() const { return attrs_; }

   //  Returns the offset to sid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Sets the corresponding member variables and adds the signal to
   //  PosixSignalRegistry.  Protected because this class is virtual.
   //
   PosixSignal(signal_t value, const char* name,
      const char* expl, uint8_t severity, const Flags& attrs);

   //  Removes the signal from PosixSignalRegistry.  Virtual to allow
   //  subclassing.
   //
   virtual ~PosixSignal();
private:
   //  Deleted to prohibit copying.
   //
   PosixSignal(const PosixSignal& that) = delete;
   PosixSignal& operator=(const PosixSignal& that) = delete;

   //  The signal's value.
   //
   signal_t value_;

   //  The signal's name (e.g. "SIGSEGV").
   //
   const char* name_;

   //  An explanation of the signal (e.g. "Invalid memory access").
   //
   const char* expl_;

   //  The signal's severity.
   //
   uint8_t severity_;

   //  The signal's attributes.
   //
   Flags attrs_;

   //  The signal's index in PosixSignalRegistry.
   //
   RegCell sid_;
};

//------------------------------------------------------------------------------
//
//  Masks for signal attributes.  These are used during system initialization,
//  so they are returned by functions because their initialization, prior to
//  usage, could not be guaranteed if they were declared using, for example,
//    const Flags PS_Native = (1 << PosixSignal::Native);
//
Flags PS_Native();
Flags PS_Break();
Flags PS_NoRecover();
Flags PS_Interrupt();
Flags PS_Delayed();
Flags PS_Exit();
Flags PS_Final();
Flags PS_NoLog();
Flags PS_NoError();
}
#endif
