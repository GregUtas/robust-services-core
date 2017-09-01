//==============================================================================
//
//  SnIncrement.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SNINCREMENT_H_INCLUDED
#define SNINCREMENT_H_INCLUDED

#include "CliIncrement.h"

using namespace NodeBase;
#include "NbTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
//  The increment that provides POTS commands.
//
class SnIncrement : public CliIncrement
{
   friend class Singleton< SnIncrement >;
private:
   //  Private because this singleton is not subclassed.
   //
   SnIncrement();

   //  Private because this singleton is not subclassed.
   //
   ~SnIncrement();
};
}
#endif