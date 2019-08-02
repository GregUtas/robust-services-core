//==============================================================================
//
//  CliAppData.h
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
#ifndef CLIAPPDATA_H_INCLUDED
#define CLIAPPDATA_H_INCLUDED

#include "Temporary.h"
#include <memory>

namespace NodeBase
{
   class CliThread;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Virtual base class for adding application-specific data to the CLI thread.
//
class CliAppData : public Temporary
{
   friend std::unique_ptr< CliAppData >::deleter_type;
public:
   //  Identifier for an application that associates data with a CLI thread.
   //  NIL_ID is used as a valid identifier.
   //
   typedef int Id;

   //> Highest valid CLI application identifier.
   //
   static const Id MaxId = 7;

   //  Events of interest to applications.
   //
   enum Event
   {
      EndOfTest
   };

   //  Returns the CLI thread associated with the data.
   //
   CliThread* Cli() const { return cli_; }

   //  Notifies the application that an event has occurred.  The default
   //  version does nothing.
   //
   virtual void EventOccurred(Event evt);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Registers the data with CLI against ID.  Protected because this
   //  class is virtual.
   //
   CliAppData(CliThread& cli, Id id);

   //  Protected to restrict deletion to CliThread.  Virtual to allow
   //  subclassing.
   //
   virtual ~CliAppData();
private:
   //  The CLI thread associated with the data.
   //
   CliThread* cli_;

   //  The application associated with the data.
   //
   const Id id_;
};

//------------------------------------------------------------------------------
//
//  Identifiers for applications that register data with a CliThread.  Each is
//  treated as a CliAppData::Id (an int) above.
//
constexpr CliAppData::Id TestcaseAppId = 0;
constexpr CliAppData::Id TestSessionAppId = 1;
}
#endif
