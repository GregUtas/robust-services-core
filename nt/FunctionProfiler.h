//==============================================================================
//
//  FunctionProfiler.h
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
#ifndef FUNCTIONPROFILER_H_INCLUDED
#define FUNCTIONPROFILER_H_INCLUDED

#include "Temporary.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include "Q2Way.h"
#include "SysTypes.h"
#include "ToolTypes.h"

namespace NodeTools
{
   class FunctionStats;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NodeTools
{
//  Generates statistics about the function calls captured during tracing.
//  After tracing has been stopped, the CLI >include command can be used
//  to change which threads are selected.  This results in a report that
//  includes only the function calls that occurred on those threads.
//
class FunctionProfiler : public Temporary
{
public:
   //  Specifies how to sort the functions.
   //
   enum Sort
   {
      ByCalls,  // sort by number of times invoked
      ByTimes,  // sort by total net time spent in function
      ByNames   // sort by name
   };

   //  Prepares to generate a report.
   //
   FunctionProfiler();

   //  Not subclassed.
   //
   ~FunctionProfiler();

   //  Builds the report and invokes Output.
   //
   TraceRc Generate(std::ostream& stream, Sort sort);
private:
   //  Searches functionq_ for FUNC's FunctionStats record, creating it
   //  if it doesn't exist.  COUNT is the number of times that FUNC has
   //  been invoked.
   //
   FunctionStats* EnsureRecord(fn_name_arg func, size_t count);

   //  Sets HIGH to CURR if CURR is greater than HIGH according to the
   //  ordered sort criteria, which have the values -1 (if HIGH < CURR),
   //  0 (if HIGH == CURR), and 1 (if HIGH > CURR).
   //
   static void CheckHigh(FunctionStats*& high, FunctionStats* curr,
      int sort1, int sort2, int sort3);

   //  Outputs the FunctionStats records after sorting them based on SORT.
   //
   TraceRc Show(std::ostream& stream, Sort sort);

   //  Overridden to prohibit copying.
   //
   FunctionProfiler(const FunctionProfiler& that);
   void operator=(const FunctionProfiler& that);

   //  The size (log2) of the functionq_ array.
   //
   static const size_t HashTableSizeLog2;

   //  The mask used to bring the hash returned by stringHash into range.
   //
   static const uint32_t HashMask;

   //  The size of the functionq_ array.
   //
   size_t size_;

   //  The hash table for FunctionStat records.  Function names with the
   //  same hash value appear in the same queue.
   //
   Q2Way< FunctionStats >* functionq_;

   //  A queue where all of the FunctionStats are placed prior to sorting.
   //
   Q2Way< FunctionStats > holdq_;

   //  The queue of sorted FunctionStats.
   //
   Q2Way< FunctionStats > sortq_;
};
}
#endif
