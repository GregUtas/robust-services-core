//==============================================================================
//
//  LibraryVarSet.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef LIBRARYVARSET_H_INCLUDED
#define LIBRARYVARSET_H_INCLUDED

#include "LibrarySet.h"
#include <string>
#include "LibraryTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  A collection of library variables.
//
class LibraryVarSet : public LibrarySet
{
public:
   //  Identifies SET with NAME.
   //
   explicit LibraryVarSet(const std::string& name);

   //  Updates RESULT with the number of library variables and returns 0.
   //
   virtual word Count(std::string& result) const override;

   //  Returns the type of set.
   //
   virtual LibSetType GetType() const override { return VAR_SET; }

   //  Displays library variable names in RESULT and returns 0.
   //
   virtual word Show(std::string& result) const override;
private:
   //  Private to restrict deletion.  Not subclassed.
   //
   ~LibraryVarSet();
};
}
#endif
