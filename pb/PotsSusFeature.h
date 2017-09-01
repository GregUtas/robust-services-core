//==============================================================================
//
//  PotsSusFeature.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSSUSFEATURE_H_INCLUDED
#define POTSSUSFEATURE_H_INCLUDED

#include "PotsFeature.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsSusFeature : public PotsFeature
{
   friend class Singleton< PotsSusFeature >;
private:
   PotsSusFeature();
   ~PotsSusFeature();
   virtual CliText* Attrs() const override;
   virtual PotsFeatureProfile* Subscribe
      (PotsProfile& profile, CliThread& cli) const override;
};
}
#endif
