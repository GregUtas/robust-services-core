//==============================================================================
//
//  StatisticsThread.h
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
#ifndef STATISTICSTHREAD_H_INCLUDED
#define STATISTICSTHREAD_H_INCLUDED

#include "Thread.h"
#include <cstddef>
#include "Duration.h"
#include "NbTypes.h"
#include "SteadyTime.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Periodically generates a statistics report and performs rollovers on
//  statistics registers.
//
class StatisticsThread : public Thread
{
   friend class Singleton<StatisticsThread>;
public:
   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   StatisticsThread();

   //  Private because this is a singleton.
   //
   ~StatisticsThread();

   //  Calculates how long the thread will sleep when it is initially
   //  entered, and also initializes countdown_;
   //
   msecs_t CalcFirstDelay();

   //  Overridden to return a name for the thread.
   //
   c_string AbbrName() const override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;

   //  Overridden to enter a loop that generates statistics reports and
   //  performs rollovers on statistics registers.
   //
   void Enter() override;

   //  Overridden to generate a statistics report during cold restarts.
   //
   bool ExitOnRestart(RestartLevel level) const override;

   //  The next time at which the thread wants to run.
   //
   SteadyTime::Point wakeupTime_;

   //  A counter that causes a report to be generated when it reaches zero.
   //
   size_t countdown_;

   //  Set when a report could not be generated and will be reattempted.
   //
   bool delayed_;
};
}
#endif
