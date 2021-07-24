//==============================================================================
//
//  StatisticsGroup.h
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
#ifndef STATISTICSGROUP_H_INCLUDED
#define STATISTICSGROUP_H_INCLUDED

#include "Dynamic.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include "NbTypes.h"
#include "RegCell.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Base class for grouping related statistics.  Statistics groups
//  survive warm restarts but must be created during all others.
//
class StatisticsGroup : public Dynamic
{
   friend class StatisticsRegistry;
public:
   //  Deleted to prohibit copying.
   //
   StatisticsGroup(const StatisticsGroup& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   StatisticsGroup& operator=(const StatisticsGroup& that) = delete;

   //> The maximum length of a string that explains a group's purpose.
   //
   static const size_t MaxExplSize;

   //> The length of a line that displays an individual statistic.
   //
   static const size_t ReportWidth;

   //  Virtual to allow subclassing.
   //
   virtual ~StatisticsGroup();

   //  Returns the group's location in the global StatisticsRegistry.
   //
   id_t Gid() const { return gid_.GetId(); }

   //  Displays statistics in STREAM.  If ID is 0, all of the group's statistics
   //  are to be displayed, else only statistics associated with the identifier
   //  (e.g. an ObjectPool identifier) are to be displayed.  The default version
   //  outputs the group's explanation and column headings, and must be invoked
   //  by subclasses.
   //
   virtual void DisplayStats
      (std::ostream& stream, id_t id, const Flags& options) const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Creates a group whose purpose is explained by EXPL.  Protected because
   //  this class is virtual.
   //
   explicit StatisticsGroup(const std::string& expl);
private:
   //  Returns the string that explains the group.
   //
   c_string Expl() const { return expl_.c_str(); }

   //  Returns the offset to gid_.
   //
   static ptrdiff_t CellDiff();

   //  The group's index in StatisticsRegistry.
   //
   RegCell gid_;

   //  An explanation of the group's statistics.
   //
   const DynamicStr expl_;
};
}
#endif
