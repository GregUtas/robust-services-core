//==============================================================================
//
//  FunctionStats.cpp
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
#include "FunctionStats.h"
#include <cstring>
#include <iomanip>
#include <ostream>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeTools
{
FunctionStats::FunctionStats(fn_name_arg func, size_t calls) :
   func_(func),
   calls_(calls),
   time_(0)
{
}

//------------------------------------------------------------------------------

FunctionStats::~FunctionStats() { }

//------------------------------------------------------------------------------

int FunctionStats::Compare(const FunctionStats& that) const
{
   auto result = this->ns_.compare(that.ns_);
   if(result != 0) return result;
   return strcmp(this->func_, that.func_);
}

//------------------------------------------------------------------------------

void FunctionStats::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << setw(9) << calls_ << spaces(2);
   stream << setw(10) << time_ << spaces(3);
   stream << func_;
   if(!ns_.empty()) stream << " (" << ns_ << ')';
   stream << CRLF;
}

//------------------------------------------------------------------------------

void FunctionStats::IncrCalls(usecs_t net)
{
   ++calls_;
   time_ += net;
}

//------------------------------------------------------------------------------

ptrdiff_t FunctionStats::LinkDiff()
{
   int local;
   auto fake = reinterpret_cast< const FunctionStats* >(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

fn_name FunctionProfiler_SetNamespace = "FunctionProfiler.SetNamespace";

void FunctionStats::SetNamespace(const string& ns)
{
   if(ns_.empty())
   {
      ns_ = ns;
      return;
   }

   ns_.push_back('/');
   ns_.append(ns);

   string expl = "Duplicate function name: ";
   expl.append(func_);
   Debug::SwErr(FunctionProfiler_SetNamespace, expl, 0, InfoLog);
}
}
