//==============================================================================
//
//  AnIncrement.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef ANINCREMENT_H_INCLUDED
#define ANINCREMENT_H_INCLUDED

#include "CliIncrement.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  The increment for Access Nodes.
//
class AnIncrement : public CliIncrement
{
   friend class Singleton< AnIncrement >;
private:
   //  Private because this singleton is not subclassed.
   //
   AnIncrement();

   //  Private because this singleton is not subclassed.
   //
   ~AnIncrement();
};
}
#endif
