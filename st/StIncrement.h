//==============================================================================
//
//  StIncrement.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef STINCREMENT_H_INCLUDED
#define STINCREMENT_H_INCLUDED

#include "CliIncrement.h"
#include "NbTypes.h"
#include "NtIncrement.h"

using namespace NodeBase;
using namespace NodeTools;

//------------------------------------------------------------------------------

namespace SessionTools
{
//  The SIZES command for this increment.
//
class StSizesCommand : public SizesCommand
{
public:
   StSizesCommand() { }
   virtual ~StSizesCommand() { }
protected:
   virtual void DisplaySizes(CliThread& cli, bool all) const override;
private:
   virtual word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------
//
//  Increment for SessionBase tools and tests.
//
class StIncrement : public CliIncrement
{
   friend class Singleton< StIncrement >;
private:
   //  Private because this singleton is not subclassed.
   //
   StIncrement();

   //  Private because this singleton is not subclassed.
   //
   ~StIncrement();

   //  Overridden to enter the increment.
   //
   virtual void Enter() override;
};
}
#endif
