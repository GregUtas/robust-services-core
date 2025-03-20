//==============================================================================
//
//  Statistics.cpp
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
#include "Statistics.h"
#include <bitset>
#include <iomanip>
#include <ostream>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "Singleton.h"
#include "StatisticsRegistry.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//> The maximum length of the string that explains the statistic.
//
constexpr size_t MaxExplSize = 40;

//  The string displayed when a value has not been set.
//
constexpr char NotUpdated = '*';

//------------------------------------------------------------------------------

fn_name Statistic_ctor = "Statistic.ctor";

Statistic::Statistic(const string& expl, size_t divisor) :
   curr_(0),
   prev_(0),
   total_(0),
   divisor_(divisor),
   expl_(expl.c_str())
{
   Debug::ft(Statistic_ctor);

   if(divisor_ == 0)
   {
      Debug::SwLog(Statistic_ctor, "invalid divisor", 0);
      divisor_ = 1;
   }

   if(expl.size() > MaxExplSize)
   {
      Debug::SwLog(Statistic_ctor, "expl length", expl.size());
   }

   Singleton<StatisticsRegistry>::Instance()->BindStat(*this);
}

//------------------------------------------------------------------------------

Statistic::~Statistic()
{
   Debug::ftnt("Statistic.dtor");

   Singleton<StatisticsRegistry>::Extant()->UnbindStat(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Statistic::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const Statistic*>(&local);
   return ptrdiff(&fake->sid_, fake);
}

//------------------------------------------------------------------------------

void Statistic::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "sid     : " << sid_.to_str() << CRLF;
   stream << prefix << "curr    : " << curr_ << CRLF;
   stream << prefix << "prev    : " << prev_ << CRLF;
   stream << prefix << "total   : " << total_ << CRLF;
   stream << prefix << "divisor : " << divisor_ << CRLF;
   stream << prefix << "expl    : " << expl_ << CRLF;
}

//------------------------------------------------------------------------------

void Statistic::DisplayStat(ostream& stream, const Flags& options) const
{
   stream << spaces(MaxExplSize + 4 - expl_.size()) << expl_;
}

//------------------------------------------------------------------------------

uint64_t Statistic::Overall() const
{
   return total_ + curr_;
}

//------------------------------------------------------------------------------

void Statistic::Patch(sel_t selector, void* arguments)
{
   Dynamic::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void Statistic::StartInterval(bool first)
{
   Debug::ft("Statistic.StartInterval");

   if(first)
      total_ = curr_.load();
   else
      total_ += curr_;

   prev_.store(curr_);
   curr_ = 0;
}

//==============================================================================

Counter::Counter(const string& expl, size_t divisor) :
   Statistic(expl, divisor)
{
   Debug::ft("Counter.ctor");
}

//------------------------------------------------------------------------------

Counter::~Counter()
{
   Debug::ftnt("Counter.dtor");
}

//------------------------------------------------------------------------------

void Counter::DisplayStat(ostream& stream, const Flags& options) const
{
   if(!options.test(DispVerbose) && (Overall() == 0)) return;

   Statistic::DisplayStat(stream, options);

   auto incr = divisor_ >> 1;

   stream << setw(10) << (curr_ + incr) / divisor_;
   stream << setw(10) << (prev_ + incr) / divisor_;
   stream << setw(12) << (Overall() + incr) / divisor_;
   stream << CRLF;
}

//------------------------------------------------------------------------------

void Counter::Patch(sel_t selector, void* arguments)
{
   Statistic::Patch(selector, arguments);
}

//==============================================================================

Accumulator::Accumulator(const string& expl, size_t divisor) :
   Counter(expl, divisor)
{
   Debug::ft("Accumulator.ctor");
}

//------------------------------------------------------------------------------

Accumulator::~Accumulator()
{
   Debug::ftnt("Accumulator.dtor");
}

//==============================================================================

HighWatermark::HighWatermark(const string& expl, size_t divisor) :
   Statistic(expl, divisor)
{
   Debug::ft("HighWatermark.ctor");

   curr_ = Initial;
   prev_ = Initial;
   total_ = Initial;
}

//------------------------------------------------------------------------------

HighWatermark::~HighWatermark()
{
   Debug::ftnt("HighWatermark.dtor");
}

//------------------------------------------------------------------------------

void HighWatermark::DisplayStat(ostream& stream, const Flags& options) const
{
   if(!options.test(DispVerbose) && (Overall() == Initial)) return;

   Statistic::DisplayStat(stream, options);

   auto incr = divisor_ >> 1;

   if(curr_ != Initial)
      stream << setw(10) << (curr_ + incr) / divisor_;
   else
      stream << setw(10) << NotUpdated;

   if(prev_ != Initial)
      stream << setw(10) << (prev_ + incr) / divisor_;
   else
      stream << setw(10) << NotUpdated;

   auto all = Overall();

   if(all != Initial)
      stream << setw(12) << (all + incr) / divisor_;
   else
      stream << setw(12) << NotUpdated;

   stream << CRLF;
}

//------------------------------------------------------------------------------

uint64_t HighWatermark::Overall() const
{
   if(total_ >= curr_) return total_;
   return curr_;
}

//------------------------------------------------------------------------------

void HighWatermark::StartInterval(bool first)
{
   Debug::ft("HighWatermark.StartInterval");

   if(first || (curr_ > total_))
   {
      total_ = curr_.load();
   }

   prev_.store(curr_);
   curr_ = Initial;
}

//==============================================================================

LowWatermark::LowWatermark(const string& expl, size_t divisor) :
   Statistic(expl, divisor)
{
   Debug::ft("LowWatermark.ctor");

   curr_ = Initial;
   prev_ = Initial;
   total_ = Initial;
}

//------------------------------------------------------------------------------

LowWatermark::~LowWatermark()
{
   Debug::ftnt("LowWatermark.dtor");
}

//------------------------------------------------------------------------------

void LowWatermark::DisplayStat(ostream& stream, const Flags& options) const
{
   if(!options.test(DispVerbose) && (Overall() == Initial)) return;

   Statistic::DisplayStat(stream, options);

   auto incr = divisor_ >> 1;

   if(curr_ != Initial)
      stream << setw(10) << (curr_ + incr) / divisor_;
   else
      stream << setw(10) << NotUpdated;

   if(prev_ != Initial)
      stream << setw(10) << (prev_ + incr) / divisor_;
   else
      stream << setw(10) << NotUpdated;

   auto all = Overall();

   if(all != Initial)
      stream << setw(12) << (all + incr) / divisor_;
   else
      stream << setw(12) << NotUpdated;

   stream << CRLF;
}

//------------------------------------------------------------------------------

uint64_t LowWatermark::Overall() const
{
   if(total_ <= curr_) return total_;
   return curr_;
}

//------------------------------------------------------------------------------

void LowWatermark::StartInterval(bool first)
{
   Debug::ft("LowWatermark.StartInterval");

   if(first || (curr_ < total_))
   {
      total_ = curr_.load();
   }

   prev_.store(curr_);
   curr_ = Initial;
}
}
