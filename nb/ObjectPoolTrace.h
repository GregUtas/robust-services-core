//==============================================================================
//
//  ObjectPoolTrace.h
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
#ifndef OBJECTPOOLTRACE_H_INCLUDED
#define OBJECTPOOLTRACE_H_INCLUDED

#include "TimedRecord.h"
#include "NbTypes.h"

namespace NodeBase
{
   class Pooled;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Records the creation or deletion of a pooled object.
//
class ObjectPoolTrace : public TimedRecord
{
public:
   static const Id Dequeued = 1;   // block returned to pool
   static const Id Enqueued = 2;   // block allocated from pool
   static const Id Claimed = 3;    // block claimed by application
   static const Id Recovered = 4;  // block recovered by audit

   //  RID is the type of event (see above), which occurred on OBJ.
   //
   ObjectPoolTrace(Id rid, const Pooled& obj);

   //  Overridden to display the trace record.
   //
   bool Display(std::ostream& stream, const std::string& opts) override;
private:
   //  Overridden to return a string explaining the event.
   //
   c_string EventString() const override;

   //  The object for which the event occurred.
   //
   const Pooled* const obj_;

   //  The pool in which the event occurred.
   //
   const ObjectPoolId pid_;
};
}
#endif
