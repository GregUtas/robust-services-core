//==============================================================================
//
//  StatisticsGroup.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
//  Base class for grouping related statistics.
//
class StatisticsGroup : public Dynamic
{
   friend class StatisticsRegistry;
public:
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

   //  Returns the offset to gid_.
   //
   static ptrdiff_t CellDiff();

   //  Displays statistics in STREAM.  If ID is 0, all of the group's statistics
   //  are to be displayed, else only statistics associated with the identifier
   //  (e.g. an ObjectPool identifier) are to be displayed.  The default version
   //  outputs the group's explanation and column headings, and must be invoked
   //  by subclasses.
   //
   virtual void DisplayStats(std::ostream& stream, id_t id) const;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Creates a group whose purpose is explained by EXPL.  Protected because
   //  this class is virtual.
   //
   explicit StatisticsGroup(const std::string& expl);
private:
   //  Overridden to prohibit copying.
   //
   StatisticsGroup(const StatisticsGroup& that);
   void operator=(const StatisticsGroup& that);

   //> The header for statistics reports.
   //
   static fixed_string ReportHeader;

   //  Returns the string that explains the group.
   //
   const char* Expl() const { return expl_.c_str(); }

   //  The group's identifier within StatisticsRegistry.
   //
   RegCell gid_;

   //  An explanation of the group's statistics.
   //
   DynString expl_;
};
}
#endif
