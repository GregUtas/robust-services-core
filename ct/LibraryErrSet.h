//==============================================================================
//
//  LibraryErrSet.h
//
//  Copyright (C) 2017  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later
//  version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef LIBRARYERRSET_H_INCLUDED
#define LIBRARYERRSET_H_INCLUDED

#include "LibrarySet.h"
#include <cstddef>
#include <string>
#include "LibraryTypes.h"
#include "SysTypes.h"

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
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
private:
   //  Private to restrict deletion.  Not subclassed.
   //
   ~LibraryErrSet();

   //  Used to explain an error in EXPL.
   //
   NodeBase::word Error(std::string& expl) const;

   //  Returns a non-zero value and updates EXPL with an explanation.
   //
   NodeBase::word Check(std::ostream* stream, std::string& expl) const override;

   //  Returns a non-zero value and updates RESULT with an explanation.
   //
   NodeBase::word Count(std::string& result) const override;

   //  Returns a non-zero value and updates RESULT with an explanation.
   //
   NodeBase::word Countlines(std::string& result) const override;

   //  Returns a non-zero value and updates EXPL with an explanation.
   //
   NodeBase::word Fix(NodeBase::CliThread& cli,
      FixOptions& opts, std::string& expl) const override;

   //  Returns a non-zero value and updates EXPL with an explanation.
   //
   NodeBase::word Format(std::string& expl) const override;

   //  Returns the type of set.
   //
   LibSetType GetType() const override { return ERR_SET; }

   //  Returns a non-zero value and updates EXPL with an explanation.
   //
   NodeBase::word List(std::ostream& stream, std::string& expl) const override;

   //  Returns a non-zero value and updates EXPL with an explanation.
   //
   NodeBase::word Parse
      (std::string& expl, const std::string& opts) const override;

   //  Returns a non-zero value and updates EXPL with an explanation.
   //
   NodeBase::word PreAssign(std::string& expl) const override;

   //  Returns a non-zero value and updates EXPL with an explanation.
   //
   NodeBase::word Scan(std::ostream& stream,
      const std::string& pattern, std::string& expl) const override;

   //  Returns a non-zero value and updates RESULT with an explanation.
   //
   NodeBase::word Show(std::string& result) const override;

   //  Returns a non-zero value and updates EXPL with an explanation.
   //
   NodeBase::word Sort(std::ostream& stream, std::string& expl) const override;

   //  The error to be reported.
   //
   const LibExprErr err_;

   //  The offset in the expression string where the error occurred.
   //
   const size_t pos_;
};
}
#endif
