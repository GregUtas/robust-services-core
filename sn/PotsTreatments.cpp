//==============================================================================
//
//  PotsTreatments.cpp
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
#include "PotsTreatments.h"
#include <bitset>
#include <cstdint>
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
PotsTreatmentQueue::PotsTreatmentQueue(QId qid)
{
   Debug::ft("PotsTreatmentQueue.ctor");

   qid_.SetId(qid);
   treatmentq_.Init(PotsTreatment::LinkDiff());

   Singleton< PotsTreatmentRegistry >::Instance()->BindTreatmentQ(*this);
}

//------------------------------------------------------------------------------

PotsTreatmentQueue::~PotsTreatmentQueue()
{
   Debug::ftnt("PotsTreatmentQueue.dtor");

   Singleton< PotsTreatmentRegistry >::Extant()->UnbindTreatmentQ(*this);
}

//------------------------------------------------------------------------------

void PotsTreatmentQueue::BindTreatment(PotsTreatment& treatment)
{
   Debug::ft("PotsTreatmentQueue.BindTreatment");

   treatmentq_.Enq(treatment);
}

//------------------------------------------------------------------------------

ptrdiff_t PotsTreatmentQueue::CellDiff()
{
   uintptr_t local;
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

PotsTreatment* PotsTreatmentQueue::FirstTreatment() const
{
   Debug::ft("PotsTreatmentQueue.FirstTreatment");

   return treatmentq_.First();
}

//------------------------------------------------------------------------------

PotsTreatment* PotsTreatmentQueue::NextTreatment
   (const PotsTreatment& treatment) const
{
   Debug::ft("PotsTreatmentQueue.NextTreatment");

   return treatmentq_.Next(treatment);
}

//------------------------------------------------------------------------------

void PotsTreatmentQueue::UnbindTreatment(PotsTreatment& treatment)
{
   Debug::ftnt("PotsTreatmentQueue.UnbindTreatment");

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
   Debug::ftnt(PotsTreatment_dtor);

   auto reg = Singleton< PotsTreatmentRegistry >::Extant();
   auto tq = reg->TreatmentQ(qid_);

   if(tq != nullptr)
      tq->UnbindTreatment(*this);
   else
      Debug::SwLog(PotsTreatment_dtor, "queue not found", qid_);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsTreatment::ApplyTreatment
   (const BcApplyTreatmentEvent& ate) const
{
   Debug::ft("PotsTreatment.ApplyTreatment");

   Context::Kill(strOver(this), pack2(ate.Owner()->Sid(), ate.GetCause()));
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
   uintptr_t local;
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

EventHandler::Rc PotsToneTreatment::ApplyTreatment
   (const BcApplyTreatmentEvent& ate) const
{
   Debug::ft("PotsToneTreatment.ApplyTreatment");

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

   stream << prefix << "tone     : " << int(tone_);
   stream << " [" << strClass(reg->GetTone(tone_), false) << ']' << CRLF;
   stream << prefix << "duration : " << duration_ << CRLF;
}

//==============================================================================

PotsLockoutTreatment::PotsLockoutTreatment(PotsTreatmentQueue::QId qid) :
   PotsTreatment(qid)
{
   Debug::ft("PotsLockoutTreatment.ctor");
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsLockoutTreatment::ApplyTreatment
   (const BcApplyTreatmentEvent& ate) const
{
   Debug::ft("PotsLockoutTreatment.ApplyTreatment");

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

PotsIdleTreatment::PotsIdleTreatment(PotsTreatmentQueue::QId qid) :
   PotsTreatment(qid)
{
   Debug::ft("PotsIdleTreatment.ctor");
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsIdleTreatment::ApplyTreatment
   (const BcApplyTreatmentEvent& ate) const
{
   Debug::ft("PotsIdleTreatment.ApplyTreatment");

   auto pssm = static_cast< PotsBcSsm* >(ate.Owner());

   return pssm->ClearCall(ate.GetCause());
}
}
