//==============================================================================
//
//  PosixSignalRegistry.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PosixSignalRegistry.h"
#include <iosfwd>
#include <sstream>
#include "Debug.h"
#include "Formatters.h"
#include "NbSignals.h"
#include "PosixSignal.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name PosixSignalRegistry_ctor = "PosixSignalRegistry.ctor";

PosixSignalRegistry::PosixSignalRegistry()
{
   Debug::ft(PosixSignalRegistry_ctor);

   signals_.Init(PosixSignal::MaxId + 1, PosixSignal::CellDiff(), MemProt);
}

//------------------------------------------------------------------------------

fn_name PosixSignalRegistry_dtor = "PosixSignalRegistry.dtor";

PosixSignalRegistry::~PosixSignalRegistry()
{
   Debug::ft(PosixSignalRegistry_dtor);
}

//------------------------------------------------------------------------------

Flags PosixSignalRegistry::Attrs(signal_t value) const
{
   if(value == SIGNIL) return Flags();

   auto s = Find(value);
   if(s == nullptr) return Flags();
   return s->Attrs();
}

//------------------------------------------------------------------------------

fn_name PosixSignalRegistry_BindSignal = "PosixSignalRegistry.BindSignal";

bool PosixSignalRegistry::BindSignal(PosixSignal& signal)
{
   Debug::ft(PosixSignalRegistry_BindSignal);

   return signals_.Insert(signal);
}

//------------------------------------------------------------------------------

void PosixSignalRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "signals [signal_t]" << CRLF;
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
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fixed_string SigUnknownStr = "Unknown Signal";

string PosixSignalRegistry::strSignal(signal_t value) const
{
   std::ostringstream stream;

   auto s = Find(value);

   stream << value;
   stream << " (";
   if(s != nullptr)
      stream << s->Name() << ": " << s->Expl();
   else
      stream << SigUnknownStr;
   stream << ')';

   return stream.str();
}

//------------------------------------------------------------------------------

fn_name PosixSignalRegistry_UnbindSignal = "PosixSignalRegistry.UnbindSignal";

void PosixSignalRegistry::UnbindSignal(PosixSignal& signal)
{
   Debug::ft(PosixSignalRegistry_UnbindSignal);

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
