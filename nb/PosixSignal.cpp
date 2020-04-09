//==============================================================================
//
//  PosixSignal.cpp
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
#include "PosixSignal.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "PosixSignalRegistry.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const signal_t PosixSignal::MaxId = UINT8_MAX;

//------------------------------------------------------------------------------

fn_name PosixSignal_ctor = "PosixSignal.ctor";

PosixSignal::PosixSignal(signal_t value, c_string name,
   c_string expl, uint8_t severity, const Flags& attrs) :
   value_(value),
   name_(name),
   expl_(expl),
   severity_(severity),
   attrs_(attrs)
{
   Debug::ft(PosixSignal_ctor);

   Singleton< PosixSignalRegistry >::Instance()->BindSignal(*this);
}

//------------------------------------------------------------------------------

fn_name PosixSignal_dtor = "PosixSignal.dtor";

PosixSignal::~PosixSignal()
{
   Debug::ft(PosixSignal_dtor);

   Singleton< PosixSignalRegistry >::Instance()->UnbindSignal(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t PosixSignal::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const PosixSignal* >(&local);
   return ptrdiff(&fake->sid_, fake);
}

//------------------------------------------------------------------------------

fixed_string AttrStrings[PosixSignal::Attribute_N] =
{
   "Native",
   "Break",
   "Interrupt",
   "Delayed",
   "Final",
   "NoLog",
   "NoError"
};

void PosixSignal::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Persistent::Display(stream, prefix, options);

   stream << prefix << "value    : " << value_ << CRLF;
   stream << prefix << "name     : " << name_ << CRLF;
   stream << prefix << "expl     : " << expl_ << CRLF;
   stream << prefix << "severity : " << int(severity_) << CRLF;
   stream << prefix << "attrs    : " << "{";

   bool found = false;

   for(auto i = 0; i < Attribute_N; ++i)
   {
      if(attrs_.test(i))
      {
         if(found) stream << SPACE;
         stream << AttrStrings[i];
         found = true;
      }
   }

   if(!found) stream << "none";
   stream << '}' << CRLF;
}

//------------------------------------------------------------------------------

void PosixSignal::Patch(sel_t selector, void* arguments)
{
   Persistent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

Flags PS_Break()
{
   return Flags(1 << PosixSignal::Break);
}

Flags PS_Delayed()
{
   return Flags(1 << PosixSignal::Delayed);
}

Flags PS_Final()
{
   return Flags(1 << PosixSignal::Final);
}

Flags PS_Interrupt()
{
   return Flags(1 << PosixSignal::Interrupt);
}

Flags PS_Native()
{
   return Flags(1 << PosixSignal::Native);
}

Flags PS_NoError()
{
   return Flags(1 << PosixSignal::NoError);
}

Flags PS_NoLog()
{
   return Flags(1 << PosixSignal::NoLog);
}
}
