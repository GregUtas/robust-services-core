//==============================================================================
//
//  LogGroupRegistry.h
//
//  Copyright (C) 2013-2020  Greg Utas
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
#ifndef LOGGROUPREGISTRY_H_INCLUDED
#define LOGGROUPREGISTRY_H_INCLUDED

#include "Immutable.h"
#include <string>
#include "NbTypes.h"
#include "Registry.h"
#include "SysTypes.h"

namespace NodeBase
{
   class Log;
   class LogGroup;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for log groups.
//
class LogGroupRegistry : public Immutable
{
   friend class Singleton< LogGroupRegistry >;
   friend class LogGroup;
public:
   //> The maximum number of log groups.
   //
   static const id_t MaxGroups;

   //  Returns the group associated with NAME.
   //
   LogGroup* FindGroup(const std::string& name) const;

   //  Returns the log associated with NAME and ID.
   //
   Log* FindLog(const std::string& name, LogId id) const;

   //  Returns the registry.
   //
   const Registry< LogGroup >& Groups() const { return groups_; }

   //  Returns the group associated with GID.
   //
   LogGroup* Group(id_t gid) const;

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
   //  Private because this singleton is not subclassed.
   //
   LogGroupRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~LogGroupRegistry();

   //  Registers GROUP.
   //
   bool BindGroup(LogGroup& group);

   //  Removes GROUP from the registry.
   //
   void UnbindGroup(LogGroup& group);

   //  The registry of log groups.
   //
   Registry< LogGroup > groups_;

   //  The statistics group for logs.
   //
   StatisticsGroupPtr statsGroup_;
};
}
#endif
