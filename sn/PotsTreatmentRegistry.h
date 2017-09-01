//==============================================================================
//
//  PotsTreatmentRegistry.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSTREATMENTREGISTRY_H_INCLUDED
#define POTSTREATMENTREGISTRY_H_INCLUDED

#include "Protected.h"
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
class PotsTreatmentRegistry : public Protected
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
   virtual void Startup(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
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
