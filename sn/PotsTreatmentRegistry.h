//==============================================================================
//
//  PotsTreatmentRegistry.h
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
#ifndef POTSTREATMENTREGISTRY_H_INCLUDED
#define POTSTREATMENTREGISTRY_H_INCLUDED

#include "Persistent.h"
#include "BcCause.h"
#include "NbTypes.h"
#include "PotsTreatments.h"
#include "Registry.h"

using namespace NodeBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Registry for treatment queues.  When a POTS call enters the Exception
//  state, it maps the cause value (the reason for call takedown) to one of
//  these queues and then applies the queue's treatments in order.  Many of
//  these treatments apply a tone while waiting for the user to go onhook.
//
class PotsTreatmentRegistry : public Persistent
{
   friend class Singleton< PotsTreatmentRegistry >;
public:
   //  Adds TREATMENTQ to the registry.
   //
   bool BindTreatmentQ(PotsTreatmentQueue& treatmentq);

   //  Removes TREATMENTQ from the registry.
   //
   void UnbindTreatmentQ(PotsTreatmentQueue& treatmentq);

   //  Sets CAUSE to map to QID.
   //
   void SetCauseToTreatmentQ(Cause::Ind cause, PotsTreatmentQueue::QId qid);

   //  Returns the queue associated with CAUSE.
   //
   PotsTreatmentQueue* CauseToTreatmentQ(Cause::Ind cause) const;

   //  Returns the queue associated with QID.
   //
   PotsTreatmentQueue* TreatmentQ(PotsTreatmentQueue::QId qid) const;

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private because this singleton is not subclassed.
   //
   PotsTreatmentRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~PotsTreatmentRegistry();

   //  The registry of treatment queues.
   //
   Registry< PotsTreatmentQueue > treatmentqs_;

   //  The cause-to-treatment mappings.
   //
   PotsTreatmentQueue::QId causeToQId_[Cause::MaxInd + 1];
};
}
#endif
