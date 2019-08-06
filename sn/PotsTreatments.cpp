//==============================================================================
//
//  PotsTreatments.cpp
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
#include "PotsTreatments.h"
#include <bitset>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "BcSessions.h"
#include "Context.h"
#include "Debug.h"
#include "Formatters.h"
#include "NbTypes.h"
#include "PotsProfile.h"
#include "PotsProtocol.h"
#include "PotsSessions.h"
#include "PotsTreatmentRegistry.h"
#include "Singleton.h"
#include "ToneRegistry.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
fn_name PotsTreatmentQueue_ctor = "PotsTreatmentQueue.ctor";

PotsTreatmentQueue::PotsTreatmentQueue(QId qid)
{
   Debug::ft(PotsTreatmentQueue_ctor);

   qid_.SetId(qid);
   treatmentq_.Init(PotsTreatment::LinkDiff());

   Singleton< PotsTreatmentRegistry >::Instance()->BindTreatmentQ(*this);
}

//------------------------------------------------------------------------------

fn_name PotsTreatmentQueue_dtor = "PotsTreatmentQueue.dtor";

PotsTreatmentQueue::~PotsTreatmentQueue()
{
   Debug::ft(PotsTreatmentQueue_dtor);

   Singleton< PotsTreatmentRegistry >::Instance()->UnbindTreatmentQ(*this);
}

//------------------------------------------------------------------------------

fn_name PotsTreatmentQueue_BindTreatment = "PotsTreatmentQueue.BindTreatment";

void PotsTreatmentQueue::BindTreatment(PotsTreatment& treatment)
{
   Debug::ft(PotsTreatmentQueue_BindTreatment);

   treatmentq_.Enq(treatment);
}

//------------------------------------------------------------------------------

ptrdiff_t PotsTreatmentQueue::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const PotsTreatmentQueue* >(&local);
   return ptrdiff(&fake->qid_, fake);
}

//------------------------------------------------------------------------------

void PotsTreatmentQueue::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   if(!options.test(DispVerbose)) return;

   stream << prefix << "qid        : " << qid_.to_str() << CRLF;
   stream << prefix << "treatmentq : " << CRLF;
   treatmentq_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name PotsTreatmentQueue_FirstTreatment = "PotsTreatmentQueue.FirstTreatment";

PotsTreatment* PotsTreatmentQueue::FirstTreatment() const
{
   Debug::ft(PotsTreatmentQueue_FirstTreatment);

   return treatmentq_.First();
}

//------------------------------------------------------------------------------

fn_name PotsTreatmentQueue_NextTreatment = "PotsTreatmentQueue.NextTreatment";

PotsTreatment* PotsTreatmentQueue::NextTreatment
   (const PotsTreatment& treatment) const
{
   Debug::ft(PotsTreatmentQueue_NextTreatment);

   return treatmentq_.Next(treatment);
}

//------------------------------------------------------------------------------

fn_name PotsTreatmentQueue_UnbindTreatment =
   "PotsTreatmentQueue.UnbindTreatment";

void PotsTreatmentQueue::UnbindTreatment(PotsTreatment& treatment)
{
   Debug::ft(PotsTreatmentQueue_UnbindTreatment);

   treatmentq_.Exq(treatment);
}

//==============================================================================

fn_name PotsTreatment_ctor = "PotsTreatment.ctor";

PotsTreatment::PotsTreatment(PotsTreatmentQueue::QId qid) : qid_(qid)
{
   Debug::ft(PotsTreatment_ctor);

   auto reg = Singleton< PotsTreatmentRegistry >::Instance();
   auto tq = reg->TreatmentQ(qid_);

   if(tq != nullptr)
      tq->BindTreatment(*this);
   else
      Debug::SwLog(PotsTreatment_ctor, "queue not found", qid_);
}

//------------------------------------------------------------------------------

fn_name PotsTreatment_dtor = "PotsTreatment.dtor";

PotsTreatment::~PotsTreatment()
{
   Debug::ft(PotsTreatment_dtor);

   auto reg = Singleton< PotsTreatmentRegistry >::Instance();
   auto tq = reg->TreatmentQ(qid_);

   if(tq != nullptr)
      tq->UnbindTreatment(*this);
   else
      Debug::SwLog(PotsTreatment_dtor, "queue not found", qid_);
}

//------------------------------------------------------------------------------

fn_name PotsTreatment_ApplyTreatment = "PotsTreatment.ApplyTreatment";

EventHandler::Rc PotsTreatment::ApplyTreatment
   (const BcApplyTreatmentEvent& ate) const
{
   Debug::ft(PotsTreatment_ApplyTreatment);

   //  This is a pure virtual function.
   //
   Context::Kill(PotsTreatment_ApplyTreatment,
      pack2(ate.Owner()->Sid(), ate.GetCause()), 0);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

void PotsTreatment::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   if(!options.test(DispVerbose)) return;

   stream << prefix << "qid  : " << int(qid_) << CRLF;
   stream << prefix << "link : " << link_.to_str() << CRLF;
}

//------------------------------------------------------------------------------

ptrdiff_t PotsTreatment::LinkDiff()
{
   int local;
   auto fake = reinterpret_cast< const PotsTreatment* >(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

fn_name PotsTreatment_NextTreatment = "PotsTreatment.NextTreatment";

PotsTreatment* PotsTreatment::NextTreatment() const
{
   Debug::ft(PotsTreatment_NextTreatment);

   auto reg = Singleton< PotsTreatmentRegistry >::Instance();
   auto tq = reg->TreatmentQ(qid_);

   if(tq == nullptr)
   {
      Debug::SwLog(PotsTreatment_NextTreatment, "queue not found", qid_);
      return nullptr;
   }

   return tq->NextTreatment(*this);
}

//==============================================================================

fn_name PotsToneTreatment_ctor = "PotsToneTreatment.ctor";

PotsToneTreatment::PotsToneTreatment
   (PotsTreatmentQueue::QId qid, Tone::Id tone, secs_t duration) :
   PotsTreatment(qid),
   tone_(tone),
   duration_(duration)
{
   Debug::ft(PotsToneTreatment_ctor);

   if(duration_ == 0)
      Debug::SwLog(PotsToneTreatment_ctor, "invalid duration", 0);
}

//------------------------------------------------------------------------------

fn_name PotsToneTreatment_ApplyTreatment = "PotsToneTreatment.ApplyTreatment";

EventHandler::Rc PotsToneTreatment::ApplyTreatment
   (const BcApplyTreatmentEvent& ate) const
{
   Debug::ft(PotsToneTreatment_ApplyTreatment);

   auto pssm = static_cast< PotsBcSsm* >(ate.Owner());
   auto upsm = PotsCallPsm::Cast(pssm->UPsm());

   upsm->SetOgTone(tone_);
   pssm->StartTimer(PotsProtocol::TreatmentTimeoutId, duration_);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

void PotsToneTreatment::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   PotsTreatment::Display(stream, prefix, options);

   if(!options.test(DispVerbose)) return;

   auto reg = Singleton< ToneRegistry >::Instance();

   stream << prefix << "tone     : " << int(tone_) ;
   stream << " [" << strClass(reg->GetTone(tone_), false) << ']' << CRLF;
   stream << prefix << "duration : " << int(duration_) << CRLF;
}

//==============================================================================

fn_name PotsLockoutTreatment_ctor = "PotsLockoutTreatment.ctor";

PotsLockoutTreatment::PotsLockoutTreatment(PotsTreatmentQueue::QId qid) :
   PotsTreatment(qid)
{
   Debug::ft(PotsLockoutTreatment_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsLockoutTreatment_ApplyTreatment =
   "PotsLockoutTreatment.ApplyTreatment";

EventHandler::Rc PotsLockoutTreatment::ApplyTreatment
   (const BcApplyTreatmentEvent& ate) const
{
   Debug::ft(PotsLockoutTreatment_ApplyTreatment);

   auto pssm = static_cast< PotsBcSsm* >(ate.Owner());
   auto upsm = PotsCallPsm::Cast(pssm->UPsm());
   auto prof = pssm->Profile();

   upsm->SendSignal(PotsSignal::Lockout);
   prof->SetState(upsm, PotsProfile::Lockout);
   pssm->SetNextSnp(BcTrigger::CallClearedSnp);
   pssm->SetNextState(BcState::Null);
   return EventHandler::Suspend;
}

//==============================================================================

fn_name PotsIdleTreatment_ctor = "PotsIdleTreatment.ctor";

PotsIdleTreatment::PotsIdleTreatment(PotsTreatmentQueue::QId qid) :
   PotsTreatment(qid)
{
   Debug::ft(PotsIdleTreatment_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsIdleTreatment_ApplyTreatment = "PotsIdleTreatment.ApplyTreatment";

EventHandler::Rc PotsIdleTreatment::ApplyTreatment
   (const BcApplyTreatmentEvent& ate) const
{
   Debug::ft(PotsIdleTreatment_ApplyTreatment);

   auto pssm = static_cast< PotsBcSsm* >(ate.Owner());

   return pssm->ClearCall(ate.GetCause());
}
}
