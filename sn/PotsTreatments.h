//==============================================================================
//
//  PotsTreatments.h
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
#ifndef POTSTREATMENTS_H_INCLUDED
#define POTSTREATMENTS_H_INCLUDED

#include "Protected.h"
#include <cstddef>
#include "Duration.h"
#include "EventHandler.h"
#include "Q1Link.h"
#include "Q1Way.h"
#include "RegCell.h"
#include "SysTypes.h"
#include "Tones.h"

namespace CallBase
{
   class BcApplyTreatmentEvent;
}

namespace PotsBase
{
   class PotsTreatment;
}

using namespace NodeBase;
using namespace SessionBase;
using namespace MediaBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  In the Exception state, the cause value (for call takedown) is mapped to a
//  queue of treatments.
//
class PotsTreatmentQueue : public Protected
{
   friend class PotsTreatment;
public:
   //  Type for identifying a treatment queue.
   //
   typedef id_t QId;

   //  Deleted to prohibit copying.
   //
   PotsTreatmentQueue(const PotsTreatmentQueue& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   PotsTreatmentQueue& operator=(const PotsTreatmentQueue& that) = delete;

   //  The treatment queues that can be selected by a cause value.
   //
   static const QId IdleQId = 1;        // sends a Release (causing dial tone)
   static const QId DisconnectQId = 2;  // sends silent tone for 10 seconds
   static const QId BusyQId = 3;        // sends busy tone
   static const QId ErrorQId = 4;       // sends reorder tone
   static const QId ConfQId = 5;        // sends confirmation tone
   static const QId MaxQId = 5;         // range constant

   //  Registers the queue against QID with PotsTreatmentRegistry.
   //
   explicit PotsTreatmentQueue(QId qid);

   //  Deregisters the queue.  Not subclassed.
   //
   ~PotsTreatmentQueue();

   //  Returns the first treatment in the queue.
   //
   PotsTreatment* FirstTreatment() const;

   //  Returns the treatment that follows TREATMENT.
   //
   PotsTreatment* NextTreatment(const PotsTreatment& treatment) const;

   //  Returns the offset to qid_.
   //
   static ptrdiff_t CellDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Adds TREATMENT to the queue.
   //
   void BindTreatment(PotsTreatment& treatment);

   //  Removes TREATMENT from the queue.
   //
   void UnbindTreatment(PotsTreatment& treatment);

   //  The queue's index in PotsTreatmentRegistry.
   //
   RegCell qid_;

   //  The queue of treatments.
   //
   Q1Way< PotsTreatment > treatmentq_;
};

//------------------------------------------------------------------------------
//
//  Base class for treatments.
//
class PotsTreatment : public Protected
{
public:
   //  Removes the treatment from its queue.  Virtual to allow subclassing.
   //
   virtual ~PotsTreatment();

   //  Deleted to prohibit copying.
   //
   PotsTreatment(const PotsTreatment& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   PotsTreatment& operator=(const PotsTreatment& that) = delete;

   //  Returns the treatment that follows this one in its queue.
   //
   PotsTreatment* NextTreatment() const;

   //  Applies the treatment during call takedown.
   //
   virtual EventHandler::Rc ApplyTreatment
      (const BcApplyTreatmentEvent& ate) const = 0;

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   //  Adds the treatment against the queue identified by QID.  Protected
   //  because this class is virtual.
   //
   explicit PotsTreatment(PotsTreatmentQueue::QId qid);
private:
   //  The identifier of the PotsTreatmentQueue in which the treatment appears.
   //
   PotsTreatmentQueue::QId qid_;

   //  The next treatment in the queue.
   //
   Q1Link link_;
};

//------------------------------------------------------------------------------
//
//  Applies a tone while waiting for the subscriber to go onhook.
//
class PotsToneTreatment : public PotsTreatment
{
public:
   //  Adds the treatment to the queue identified by QID.  TONE is the tone to
   //  apply.  It lasts for DURATION before advancing to the next treatment in
   //  the queue.
   //
   PotsToneTreatment
      (PotsTreatmentQueue::QId qid, Tone::Id tone, secs_t duration);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Applies tone_ and starts a timer that expires in duration_ seconds.
   //
   EventHandler::Rc ApplyTreatment
      (const BcApplyTreatmentEvent& ate) const override;

   //  The tone that the treatment applies.
   //
   Tone::Id tone_;

   //  The duration of tone_.
   //
   secs_t duration_;
};

//------------------------------------------------------------------------------
//
//  Puts the POTS circuit in the lockout state when all treatments have been
//  applied and an onhook has still not occurred.  This frees all resources
//  (SSM, PSM) and effectively makes the circuit busy until it goes onhook.
//
class PotsLockoutTreatment : public PotsTreatment
{
public:
   //  Adds the treatment to the queue identified by QID.
   //
   explicit PotsLockoutTreatment(PotsTreatmentQueue::QId qid);
private:
   //  Sends a Lockout message and puts the circuit in the lockout state.
   //
   EventHandler::Rc ApplyTreatment
      (const BcApplyTreatmentEvent& ate) const override;
};

//------------------------------------------------------------------------------
//
//  Sends a Release message to the circuit.  This is done after an unexpected
//  error.  Because the circuit is offhook, it will immediately respond with
//  an Offhook message, resulting in dial tone.  The subscriber will then know
//  that the call ended abnormally.
//
class PotsIdleTreatment : public PotsTreatment
{
public:
   //  Adds the treatment to the queue identified by QID.
   //
   explicit PotsIdleTreatment(PotsTreatmentQueue::QId qid);
private:
   //  Sends a Release message and puts the circuit in the idle state.
   //
   EventHandler::Rc ApplyTreatment
      (const BcApplyTreatmentEvent& ate) const override;
};
}
#endif
