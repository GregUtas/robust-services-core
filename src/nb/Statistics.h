//==============================================================================
//
//  Statistics.h
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
#ifndef STATISTICS_H_INCLUDED
#define STATISTICS_H_INCLUDED

#include "Dynamic.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "NbTypes.h"
#include "RegCell.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Base class for statistics.  Statistics survive warm restarts
//  but must be created during all others.
//
class Statistic : public Dynamic
{
   friend class StatisticsRegistry;
public:
   //  Deleted to prohibit copying.
   //
   Statistic(const Statistic& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   Statistic& operator=(const Statistic& that) = delete;

   //  Virtual to allow subclassing.
   //
   virtual ~Statistic();

   //  Returns the value during the current measurement period.
   //
   size_t Curr() const { return curr_; }

   //  Returns the value over all measurement periods.
   //
   virtual uint64_t Overall() const;

   //  Display the statistic in STREAM.
   //
   virtual void DisplayStat(std::ostream& stream, const Flags& options) const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Creates a statistic which is explained by EXPL.  To support
   //  scaling, values are divided by DIVISOR before being displayed
   //  in statistics reports.  Protected because this class is virtual.
   //
   Statistic(const std::string& expl, size_t divisor);

   //  The statistic's value during the current measurement period.
   //
   std::atomic_size_t curr_;

   //  The statistic's value during the previous measurement period.
   //
   std::atomic_size_t prev_;

   //  The statistic's value over all measurement periods.
   //
   std::atomic_uint64_t total_;

   //  The divisor used when displaying totals.
   //
   size_t divisor_;
private:
   //  Invoked at regular intervals to start a new measurement period.
   //  If FIRST is true, previous values in total_ are discarded.  The
   //  default version adds curr_ to total_, sets prev_ to curr_, and
   //  zeroes curr_, and must be overridden by subclasses that require
   //  different behavior.
   //
   virtual void StartInterval(bool first);

   //  Returns the offset to mid_.
   //
   static ptrdiff_t CellDiff();

   //  The statistic's index in StatisticsRegistry.
   //
   RegCell sid_;

   //  The string that explains the statistic's purpose.
   //
   const DynamicStr expl_;
};

//------------------------------------------------------------------------------
//
//  Counts how many times an event has occurred.
//
class Counter : public Statistic
{
public:
   //  Public so that instances can be created as members.
   //
   explicit Counter(const std::string& expl, size_t divisor = 1);

   //  Virtual to allow subclassing.
   //
   virtual ~Counter();

   //  Increments the count and returns it.
   //
   size_t Incr() { return ++curr_; }

   //  Overridden to display the statistic.
   //
   void DisplayStat(std::ostream& stream, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
};

//------------------------------------------------------------------------------
//
//  Maintains a running total.
//
class Accumulator : public Counter
{
public:
   //  Public so that instances can be created as members.
   //
   explicit Accumulator(const std::string& expl, size_t divisor = 1);

   //  Virtual to allow subclassing.
   //
   virtual ~Accumulator();

   //  Updates the total and returns it.
   //
   size_t Add(size_t count) { return (curr_ += count); }
};

//------------------------------------------------------------------------------
//
//  Tracks a maximum value.
//
class HighWatermark : public Statistic
{
public:
   //  The initial value for the watermark.
   //
   static const size_t Initial = 0;

   //  Public so that instances can be created as members.
   //
   explicit HighWatermark(const std::string& expl, size_t divisor = 1);

   //  Virtual to allow subclassing.
   //
   virtual ~HighWatermark();

   //  Updates the watermark.
   //
   void Update(size_t count) { if(count > curr_) curr_ = count; }

   //  Overridden to display the statistic.
   //
   void DisplayStat(std::ostream& stream, const Flags& options) const override;

   //  Overridden to return the value over all measurement periods.
   //
   uint64_t Overall() const override;
private:
   //  Overridden to start a new measurement interval.
   //
   void StartInterval(bool first) override;
};

//------------------------------------------------------------------------------
//
//  Tracks a minimum value.
//
class LowWatermark : public Statistic
{
public:
   //  The initial value for the watermark.
   //
   static const size_t Initial = SIZE_MAX;

   //  Public so that instances can be created as members.
   //
   explicit LowWatermark(const std::string& expl, size_t divisor = 1);

   //  Virtual to allow subclassing.
   //
   virtual ~LowWatermark();

   //  Updates the watermark.
   //
   void Update(size_t count) { if(count < curr_) curr_ = count; }

   //  Overridden to display the statistic.
   //
   void DisplayStat(std::ostream& stream, const Flags& options) const override;

   //  Overridden to return the value over all measurement periods.
   //
   uint64_t Overall() const override;
private:
   //  Overridden to start a new measurement interval.
   //
   void StartInterval(bool first) override;
};
}
#endif
