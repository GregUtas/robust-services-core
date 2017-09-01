//==============================================================================
//
//  CodeIncrement.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CODEINCREMENT_H_INCLUDED
#define CODEINCREMENT_H_INCLUDED

#include "CliIncrement.h"
#include "NbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Increment for source code analysis.
//
class CodeIncrement : public CliIncrement
{
   friend class Singleton< CodeIncrement >;
private:
   //  Private because this singleton is not subclassed.
   //
   CodeIncrement();

   //  Private because this singleton is not subclassed.
   //
   ~CodeIncrement();
};
}
#endif
