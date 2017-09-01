//==============================================================================
//
//  SbTypes.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "SbTypes.h"

using std::ostream;

//------------------------------------------------------------------------------

namespace SessionBase
{
fixed_string ContextTypeStrings[ContextType_N + 1] =
{
   "fac",  // SingleMsg
   "psm",  // SinglePort
   "ssm",  // MultiPort
   "???"
};

ostream& operator<<(ostream& stream, ContextType type)
{
   if((type >= 0) && (type < ContextType_N))
      stream << ContextTypeStrings[type];
   else
      stream << ContextTypeStrings[ContextType_N];
   return stream;
}

const char* strContextType(ContextType type)
{
   if((type >= 0) && (type < ContextType_N)) return ContextTypeStrings[type];
   return ContextTypeStrings[ContextType_N];
}
}
