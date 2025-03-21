//==============================================================================
//
//  PosixSignalRegistry.cpp
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
#include "PosixSignalRegistry.h"
#include <bitset>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <sstream>
#include "Debug.h"
#include "Formatters.h"
#include "NbSignals.h"
#include "PosixSignal.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
PosixSignalRegistry::PosixSignalRegistry()
{
   Debug::ft("PosixSignalRegistry.ctor");

   signals_.Init(PosixSignal::MaxId, PosixSignal::CellDiff(), MemImmutable);
}

//------------------------------------------------------------------------------

fn_name PosixSignalRegistry_dtor = "PosixSignalRegistry.dtor";

PosixSignalRegistry::~PosixSignalRegistry()
{
   Debug::ftnt(PosixSignalRegistry_dtor);

   Debug::SwLog(PosixSignalRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

Flags PosixSignalRegistry::Attrs(signal_t value) const
{
   if(value == SIGNIL) return NoFlags;

   auto s = Find(value);
   if(s == nullptr) return NoFlags;
   return s->Attrs();
}

//------------------------------------------------------------------------------

bool PosixSignalRegistry::BindSignal(PosixSignal& signal)
{
   Debug::ft("PosixSignalRegistry.BindSignal");

   return signals_.Insert(signal);
}

//------------------------------------------------------------------------------

void PosixSignalRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "signals [id_t]" << CRLF;
   signals_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

PosixSignal* PosixSignalRegistry::Find(signal_t value) const
{
   for(auto s = signals_.First(); s != nullptr; signals_.Next(s))
   {
      if(s->Value() == value) return s;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

PosixSignal* PosixSignalRegistry::Find(const string& name) const
{
   for(auto s = signals_.First(); s != nullptr; signals_.Next(s))
   {
      if(s->Name() == name) return s;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void PosixSignalRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fixed_string SigNilStr = "Normal Exit";
fixed_string SigUnknownStr = "Unknown Signal";

string PosixSignalRegistry::strSignal(signal_t value) const
{
   std::ostringstream stream;

   auto s = Find(value);

   stream << value;
   stream << " (";
   if(s != nullptr)
      stream << s->Name() << ": " << s->Expl();
   else if(value == SIGNIL)
      stream << SigNilStr;
   else
      stream << SigUnknownStr;
   stream << ')';

   return stream.str();
}

//------------------------------------------------------------------------------

fixed_string SignalHeader = " Id  Name        Explanation";
//                          |  3..11         .<expl>

size_t PosixSignalRegistry::Summarize(ostream& stream, uint32_t selector) const
{
   stream << SignalHeader << CRLF;

   for(auto s = signals_.First(); s != nullptr; signals_.Next(s))
   {
      stream << setw(3) << s->Value();
      stream << spaces(2) << std::left << setw(11) << s->Name();
      stream << SPACE << std::right << s->Expl() << CRLF;
   }

   return signals_.Size();
}

//------------------------------------------------------------------------------

void PosixSignalRegistry::UnbindSignal(PosixSignal& signal)
{
   Debug::ftnt("PosixSignalRegistry.UnbindSignal");

   signals_.Erase(signal);
}

//------------------------------------------------------------------------------

signal_t PosixSignalRegistry::Value(const string& name) const
{
   auto s = Find(name);
   if(s == nullptr) return SIGNIL;
   if(s->Attrs().test(PosixSignal::Native)) return s->Value();
   return SIGNIL;
}
}
