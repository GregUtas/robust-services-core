//==============================================================================
//
//  LogGroup.h
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
#ifndef LOGGROUP_H_INCLUDED
#define LOGGROUP_H_INCLUDED

#include "Immutable.h"
#include <cstddef>
#include <iosfwd>
#include "NbTypes.h"
#include "RegCell.h"
#include "Registry.h"
#include "SysTypes.h"

namespace NodeBase
{
   class Log;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Base class for grouping related logs.  Log groups survive all
//  restarts so that logs can be generated during a restart.
//
class LogGroup : public Immutable
{
   friend class Log;
public:
   //> The maximum length of a log group's name.
   //
   static const size_t MaxNameSize;

   //> The maximum length of the string that explains a log group.
   //
   static const size_t MaxExplSize;

   //> The maximum number of logs in a group.
   //
   static const id_t MaxLogs;

   //  Creates a group identified by NAME, which is converted to
   //  upper case.  EXPL describes the types of logs in the group.
   //
   LogGroup(fixed_string name, fixed_string expl);

   //  Not subclassed.
   //
   ~LogGroup();

   //  Deleted to prohibit copying.
   //
   LogGroup(const LogGroup& that) = delete;
   LogGroup& operator=(const LogGroup& that) = delete;

   //  Returns the group's name.
   //
   c_string Name() const { return name_.c_str(); }

   //  Returns the group's identifier.
   //
   id_t Gid() const { return gid_.GetId(); }

   //  Returns true if all logs in the group are to be suppressed.
   //
   bool Suppressed() const { return suppressed_; }

   //  Controls whether all logs in the group are to be suppressed.
   //
   void SetSuppressed(bool suppressed);

   //  Returns the log associated with ID.
   //
   Log* FindLog(LogId id) const;

   //  Displays the statistics for each log in the group.
   //
   void DisplayStats(std::ostream& stream, const Flags& options) const;

   //  Returns the offset to gid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Adds LOG to the group.
   //
   bool BindLog(Log& log);

   //  Removes LOG from the group.
   //
   void UnbindLog(Log& log);

   //  The group's name.
   //
   const ImmutableStr name_;

   //  The group's explanation.
   //
   const ImmutableStr expl_;

   //  Set if all logs in the group are to be suppressed.
   //
   bool suppressed_;

   //  The group's index in LogGroupRegistry.
   //
   RegCell gid_;

   //  The logs in the group.
   //
   Registry< Log > logs_;
};
}
#endif
