//==============================================================================
//
//  LibraryErrSet.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef LIBRARYERRSET_H_INCLUDED
#define LIBRARYERRSET_H_INCLUDED

#include "LibrarySet.h"
#include <cstddef>
#include <string>
#include "LibraryTypes.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Create to report an error associated with a library set.
//
class LibraryErrSet : public LibrarySet
{
public:
   //  Creates a set identified by NAME.  It will be a temporary variable
   //  to report ERR.  POS is the offset in the expression string where
   //  the error occurred.
   //
   LibraryErrSet(const std::string& name, LibExprErr err, size_t pos);

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private to restrict deletion.  Not subclassed.
   //
   ~LibraryErrSet();

   //  Used to explain an error in EXPL.
   //
   word Error(std::string& expl) const;

   //  Returns a non-zero value and updates EXPL with an explanation.
   //
   virtual word Check(std::ostream& stream, std::string& expl) const override;

   //  Returns a non-zero value and updates RESULT with an explanation.
   //
   virtual word Count(std::string& result) const override;

   //  Returns a non-zero value and updates RESULT with an explanation.
   //
   virtual word Countlines(std::string& result) const override;

   //  Returns a non-zero value and updates EXPL with an explanation.
   //
   virtual word Format(std::string& expl) const override;

   //  Returns the type of set.
   //
   virtual LibSetType GetType() const override { return ERR_SET; }

   //  Returns a non-zero value and updates EXPL with an explanation.
   //
   virtual word List(std::ostream& stream, std::string& expl) const override;

   //  Returns a non-zero value and updates EXPL with an explanation.
   //
   virtual word Parse
      (std::string& expl, const std::string& opts) const override;

   //  Returns a non-zero value and updates EXPL with an explanation.
   //
   virtual word PreAssign(std::string& expl) const override;

   //  Returns a non-zero value and updates EXPL with an explanation.
   //
   virtual word Scan(std::ostream& stream,
      const std::string& pattern, std::string& expl) const override;

   //  Returns a non-zero value and updates RESULT with an explanation.
   //
   virtual word Show(std::string& result) const override;

   //  Returns a non-zero value and updates EXPL with an explanation.
   //
   virtual word Sort(std::ostream& stream, std::string& expl) const override;

   //  Returns a non-zero value and updates EXPL with an explanation.
   //
   virtual word Trim(std::ostream& stream, std::string& expl) const override;

   //  The error to be reported.
   //
   const LibExprErr err_;

   //  The offset in the expression string where the error occurred.
   //
   const size_t pos_;
};
}
#endif
