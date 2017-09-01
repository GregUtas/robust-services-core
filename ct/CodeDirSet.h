//==============================================================================
//
//  CodeDirSet.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef CODEDIRSET_H_INCLUDED
#define CODEDIRSET_H_INCLUDED

#include "CodeSet.h"
#include <string>
#include "LibraryTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  A set of code directories.
//
class CodeDirSet : public CodeSet
{
public:
   //  Identifies SET with NAME.
   //
   CodeDirSet(const std::string& name, SetOfIds* set);

   //  Override the operators supported by a set of code directories.
   //
   virtual LibrarySet* Create
      (const std::string& name, SetOfIds* set) const override;
   virtual LibrarySet* Files() const override;

   //  Returns the type of set.
   //
   virtual LibSetType GetType() const override { return DIR_SET; }

   //  Displays the full directory paths in STREAM and returns 0.
   //
   virtual word List(std::ostream& stream, std::string& expl) const override;

   //  Displays the directory names in RESULT and returns 0.
   //
   virtual word Show(std::string& result) const override;
private:
   //  Private to restrict deletion.  Not subclassed.
   //
   ~CodeDirSet();
};
}
#endif
