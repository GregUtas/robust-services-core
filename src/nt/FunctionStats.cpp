//==============================================================================
//
//  FunctionStats.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "FunctionStats.h"
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <ostream>
#include <ratio>
#include <string>
#include "Algorithms.h"
#include "Formatters.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeTools
{
FunctionStats::FunctionStats(fn_name_arg func, size_t calls) :
   func_(func),
   calls_(calls)
{
}

//------------------------------------------------------------------------------

int FunctionStats::Compare(const FunctionStats& that) const
{
   return strcmp(this->func_, that.func_);
}

//------------------------------------------------------------------------------

void FunctionStats::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << setw(9) << calls_ << spaces(2);
   stream << setw(10) << time_.count() << spaces(3);
   stream << func_;
   stream << CRLF;
}

//------------------------------------------------------------------------------

void FunctionStats::IncrCalls(const usecs_t& net)
{
   ++calls_;
   time_ += net;
}

//------------------------------------------------------------------------------

ptrdiff_t FunctionStats::LinkDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const FunctionStats*>(&local);
   return ptrdiff(&fake->link_, fake);
}
}
