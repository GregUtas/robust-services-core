//==============================================================================
//
//  ObjectPoolTrace.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
   static const Id Dequeued  = 1;  // block returned to pool
   static const Id Enqueued  = 2;  // block allocated from pool
   static const Id Claimed   = 3;  // block claimed by application
   static const Id Recovered = 4;  // block recovered by audit

   //  RID is the type of event (see above), which occurred on OBJ.
   //
   ObjectPoolTrace(Id rid, const Pooled& obj);

   //  Overridden to display the trace record.
   //
   virtual bool Display(std::ostream& stream) override;
private:
   //  Overridden to return a string explaining the event.
   //
   virtual const char* EventString() const override;

   //  The object for which the event occurred.
   //
   const Pooled* const obj_;

   //  The pool in which the event occurred.
   //
   const ObjectPoolId pid_;
};
}
#endif
