//==============================================================================
//
//  FunctionStats.h
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
#ifndef FUNCTIONSTATS_H_INCLUDED
#define FUNCTIONSTATS_H_INCLUDED

#include "Temporary.h"
#include <cstddef>
#include "Clock.h"
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
   ~FunctionStats();

   //  Returns a pointer to the function's name.
   //
   fn_name Func() const { return func_; }

   //  Returns the number of times the function was invoked.
   //
   size_t Calls() const { return calls_; }

   //  Returns the total net time spent in the function.
   //
   msecs_t Time() const { return time_; }

   //  Increments the number of times the function was invoked and
   //  adds NET to the total net time spent in it.
   //
   void IncrCalls(usecs_t net);

   //  Overridden to display the function's statistics.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();
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
   usecs_t time_;
};
}
#endif
