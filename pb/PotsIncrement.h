//==============================================================================
//
//  PotsIncrement.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSINCREMENT_H_INCLUDED
#define POTSINCREMENT_H_INCLUDED

#include "CliIncrement.h"
#include "NbTypes.h"
#include "StIncrement.h"

using namespace NodeBase;
using namespace SessionTools;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  The SIZES command for this increment.
//
class PbSizesCommand : public StSizesCommand
{
public:
   PbSizesCommand() { }
   virtual ~PbSizesCommand() { }
protected:
   virtual void DisplaySizes(CliThread& cli, bool all) const override;
private:
   virtual word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------
//
//  The increment that provides POTS commands.
//
class PotsIncrement : public CliIncrement
{
   friend class Singleton< PotsIncrement >;
private:
   //  Private because this singleton is not subclassed.
   //
   PotsIncrement();

   //  Private because this singleton is not subclassed.
   //
   ~PotsIncrement();
};
}
#endif
