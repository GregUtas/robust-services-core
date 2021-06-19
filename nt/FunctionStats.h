//==============================================================================
//
//  FunctionStats.h
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
#ifndef FUNCTIONSTATS_H_INCLUDED
#define FUNCTIONSTATS_H_INCLUDED

#include "Temporary.h"
#include <cstddef>
#include "Duration.h"
#include "Q2Link.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NodeTools
{
//  Statistics about a function's invocations.
//
class FunctionStats : public Temporary
{
public:
   //  Creates an entry for FUNC, starting with CALLS invocations.
   //
   FunctionStats(fn_name_arg func, size_t calls);

   //  Not subclassed.
   //
   ~FunctionStats() = default;

   //  Deleted to prohibit copying.
   //
   FunctionStats(const FunctionStats& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   FunctionStats& operator=(const FunctionStats& that) = delete;

   //  Returns a pointer to the function's name.
   //
   fn_name Func() const { return func_; }

   //  Returns the number of times the function was invoked.
   //
   size_t Calls() const { return calls_; }

   //  Returns the total net time spent in the function.
   //
   Duration Time() const { return time_; }

   //  Increments the number of times the function was invoked and
   //  adds NET to the total net time spent in it.
   //
   void IncrCalls(const Duration& net);

   //  Returns -1, 0, or 1 if THAT is less than, equal to, or greater
   //  than "this" when sorted by namespace and function name.
   //
   int Compare(const FunctionStats& that) const;

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

   //  Overridden to display the function's statistics.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  The two-way queue link for FunctionProfiler queues.
   //
   Q2Link link_;

   //  The function's name.
   //
   fn_name func_;

   //  The number of times that the function was invoked.
   //
   size_t calls_;

   //  The total net time spent in the function.
   //
   Duration time_;
};
}
#endif
