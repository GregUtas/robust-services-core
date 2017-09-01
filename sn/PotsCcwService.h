//==============================================================================
//
//  PotsCcwService.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSCCWSERVICE_H_INCLUDED
#define POTSCCWSERVICE_H_INCLUDED

#include "NbTypes.h"
#include "Service.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCcwService : public Service
{
   friend class Singleton< PotsCcwService >;
private:
   PotsCcwService();
   ~PotsCcwService();
   virtual ServiceSM* AllocModifier() const override;
};
}
#endif
