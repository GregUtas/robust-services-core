//==============================================================================
//
//  Statistics.h
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

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Base class for statistics.
//
class Statistic : public Dynamic
{
   friend class StatisticsRegistry;
public:
   //> The maximum length of the string that explains the statistic.
   //
   static const size_t MaxExplSize;

   //  Virtual to allow subclassing.
   //
   virtual ~Statistic();

   //  Returns the value during the current measurement period.
   //
   uint32_t Curr() const { return curr_; }

   //  Returns the value over all measurement periods.
   //
   virtual uint64_t Overall() const;

   //  Display the statistic in STREAM.
   //
   virtual void DisplayStat(std::ostream& stream) const;

   //  Returns the offset to mid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Creates a statistic which is explained by EXPL.  To support
   //  scaling, values are divided by DIVISOR before being displayed
   //  in statistics reports.  Protected because this class is virtual.
   //
   Statistic(const std::string& expl, uint32_t divisor);

   //  The string displayed when a value has not been set.
   //
   static const char NotUpdated;

   //  The statistic's value during the current measurement period.
   //
   std::atomic_uint32_t curr_;

   //  The statistic's value during the previous measurement period.
   //
   std::atomic_uint32_t prev_;

   //  The statistic's value over all measurement periods.
   //
   std::atomic_uint64_t total_;

   //  The divisor used when displaying totals.
   //
   uint32_t divisor_;
private:
   //  Overridden to prohibit copying.
   //
   Statistic(const Statistic& that);
   void operator=(const Statistic& that);

   //  Invoked at regular intervals to start a new measurement period.
   //  If FIRST is true, previous values in total_ are discarded.  The
   //  default version adds curr_ to total_, sets prev_ to curr_, and
   //  zeroes curr_, and must be overridden by subclasses that require
   //  different behavior.
   //
   virtual void StartInterval(bool first);

   //  The statistic's identifier within StatisticsRegistry.
   //
   RegCell sid_;

   //  The string that explains the statistic's purpose.
   //
   DynString expl_;
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
   explicit Counter(const std::string& expl, uint32_t divisor = 1);

   //  Virtual to allow subclassing.
   //
   virtual ~Counter();

   //  Increments the count and returns it.
   //
   uint32_t Incr() { return ++curr_; }

   //  Overridden to display the statistic.
   //
   virtual void DisplayStat(std::ostream& stream) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
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
   explicit Accumulator(const std::string& expl, uint32_t divisor = 1);

   //  Virtual to allow subclassing.
   //
   virtual ~Accumulator();

   //  Updates the total and returns it.
   //
   uint32_t Add(uint32_t amount) { return (curr_ += amount); }
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
   static const uint32_t Initial = 0;

   //  Public so that instances can be created as members.
   //
   explicit HighWatermark(const std::string& expl, uint32_t divisor = 1);

   //  Virtual to allow subclassing.
   //
   virtual ~HighWatermark();

   //  Updates the watermark.
   //
   void Update(uint32_t count) { if(count > curr_) curr_ = count; }

   //  Overridden to return the value over all measurement periods.
   //
   virtual uint64_t Overall() const override;

   //  Overridden to display the statistic.
   //
   virtual void DisplayStat(std::ostream& stream) const override;
private:
   //  Overridden to start a new measurement interval.
   //
   virtual void StartInterval(bool first) override;
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
   static const uint32_t Initial = UINT32_MAX;

   //  Public so that instances can be created as members.
   //
   explicit LowWatermark(const std::string& expl, uint32_t divisor = 1);

   //  Virtual to allow subclassing.
   //
   virtual ~LowWatermark();

   //  Updates the watermark.
   //
   void Update(uint32_t count) { if(count < curr_) curr_ = count; }

   //  Overridden to return the value over all measurement periods.
   //
   virtual uint64_t Overall() const override;

   //  Overridden to display the statistic.
   //
   virtual void DisplayStat(std::ostream& stream) const override;
private:
   //  Overridden to start a new measurement interval.
   //
   virtual void StartInterval(bool first) override;
};
}
#endif
