//==============================================================================
//
//  LibrarySet.h
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
#ifndef LIBRARYSET_H_INCLUDED
#define LIBRARYSET_H_INCLUDED

#include "LibraryItem.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "LibraryTypes.h"
#include "Q2Link.h"
#include "SysTypes.h"

namespace NodeBase
{
   class CliThread;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  A collection of library items (code files or directories).
//
class LibrarySet : public LibraryItem
{
   friend class Library;
   friend class Q2Way< LibrarySet >;
public:
   //> Prefix for a read-only set.
   //
   static const char ReadOnlyChar;

   //> Prefix for a temporary set.
   //
   static const char TemporaryChar;

   //> Returns a name for a temporary variable.
   //
   static std::string TemporaryName();

   //  Returns true if the set is read-only for CLI commands.
   //
   bool IsReadOnly() const;

   //  Returns true if the set is a temporary result that will not be saved.
   //
   bool IsTemporary() const;

   //  Returns the type of set.  Must be overridden by subclasses.
   //
   virtual LibSetType GetType() const;

   //  Returns 0 after checking code files in the set for conformance to
   //  C++ coding guidelines.  If STREAM is not nullptr, produces a report
   //  that contains line counts and warnings.
   //
   virtual word Check(std::ostream* stream, std::string& expl) const;

   //  On success, returns 0 and updates RESULT with the number of items
   //  in the set.  Returns another value on failure and updates RESULT
   //  with an explanation.
   //
   virtual word Count(std::string& result) const;

   //  On success, returns 0 and updates RESULT with the number of lines
   //  of code in the set.  Returns another value on failure and updates
   //  RESULT with an explanation.
   //
   virtual word Countlines(std::string& result) const;

   //  Returns 0 after fixing warnings detected by Check() in the set.
   //  Returns another value on failure and updates EXPL with an explanation.
   //
   virtual word Fix(CliThread& cli, std::string& expl) const;

   //  On success, returns 0 after reformatting the file.  Returns another
   //  value on failure and updates EXPL with an explanation.
   //
   virtual word Format(std::string& expl) const;

   //  On success, returns 0 and updates STREAM with a list of the items in
   //  the set, one per line.  Returns another value on failure and updates
   //  EXPL with an explanation.
   //
   virtual word List(std::ostream& stream, std::string& expl) const;

   //  On success, returns 0 after parsing items in the set.  EXPL describes
   //  the outcome.  The first character in TRACE indicates whether to create
   //  a parse file never ('n'), on failures only ('f'), or always ('a').
   //  The second character indicates whether to create an execution file
   //  never ('n') or always ('a').
   //
   virtual word Parse(std::string& expl, const std::string& opts) const;

   //  On success, returns 0 and updates STREAM with lines in the set that
   //  match PATTERN.  Returns another value on failure and updates EXPL
   //  with an explanation.
   //
   virtual word Scan(std::ostream& stream,
      const std::string& pattern, std::string& expl) const;

   //  On success, returns 0 and updates RESULT with the items in the set,
   //  separated by commas.  Returns another value on failure and updates
   //  RESULT with an explanation.
   //
   virtual word Show(std::string& result) const;

   //  On success, returns 0 and updates STREAM with the build order of
   //  the set.  Returns another value on failure and updates EXPL with
   //  an explanation.
   //
   virtual word Sort(std::ostream& stream, std::string& expl) const;

   //  Returns 0 after producing, in STREAM, a report about the symbols used
   //  by each item in the set, along with the #includes, forwards and using
   //  statements that should be added or removed.
   //
   virtual word Trim(std::ostream& stream, std::string& expl) const;

   //  Deletes the set unless it is registered on the queue of sets.
   //
   void Release();

   //  Operators.  The default implementations invoke OpError (see below)
   //  and must be overridden by a subclass that supports the operator.
   //
   virtual LibrarySet* Create(const std::string& name, SetOfIds* set) const;
   virtual LibrarySet* Assign(LibrarySet* rhs);
   virtual LibrarySet* Intersection(const LibrarySet* rhs) const;
   virtual LibrarySet* Difference(const LibrarySet* rhs) const;
   virtual LibrarySet* Union(const LibrarySet* rhs) const;
   virtual LibrarySet* Directories() const;
   virtual LibrarySet* Files() const;
   virtual LibrarySet* FileName(const LibrarySet* that) const;
   virtual LibrarySet* FileType(const LibrarySet* that) const;
   virtual LibrarySet* MatchString(const LibrarySet* that) const;
   virtual LibrarySet* FoundIn(const LibrarySet* that) const;
   virtual LibrarySet* Implements() const;
   virtual LibrarySet* UsedBy(bool self) const;
   virtual LibrarySet* Users(bool self) const;
   virtual LibrarySet* AffectedBy() const;
   virtual LibrarySet* Affecters() const;
   virtual LibrarySet* CommonAffecters() const;
   virtual LibrarySet* NeededBy() const;
   virtual LibrarySet* Needers() const;

   //  Returns the offset to link_.
   //
   static ptrdiff_t LinkDiff();

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   //  Creates a set that with the identifier NAME.  If NAME is not prefixed
   //  by TemporaryChar, it is added to the set of library variables.  If it
   //  is prefixed by ReadOnlyChar, it is treated as read-only.  Protected
   //  because this class is virtual.
   //
   explicit LibrarySet(const std::string& name);

   //  Removes the set from the queue of sets.  Protected to restrict deletion.
   //
   virtual ~LibrarySet();

   //  Reports COUNT items in RESULT and returns 0.
   //
   static word Counted(std::string& result, const size_t* count);

   //  If RESULT is not empty, deletes a presumed trailing ", ", else sets
   //  RESULT to indicate that nothing was found.  Returns 0.
   //
   static word Shown(std::string& result);
private:
   //  Overridden to prohibit copying.
   //
   LibrarySet(const LibrarySet& that);
   void operator=(const LibrarySet& that);

   //  Returns 0 if this set can be assigned to a variable.  Returns another
   //  value and updates EXPL with an explanation if it cannot be assigned.
   //
   virtual word PreAssign(std::string& expl) const;

   //  Update EXPL to indicate that this function is not implemented by the
   //  type of set in question.
   //
   word NotImplemented(std::string& expl) const;

   //  Generates a log and returns nullptr.
   //
   LibrarySet* OpError() const;

   //  Link for the queue of sets.
   //
   Q2Link link_;

   //  Set for a temporary variable.
   //
   bool temp_;

   //  Sequence number for temporary variables.
   //
   static uint8_t SeqNo_;
};
}
#endif
