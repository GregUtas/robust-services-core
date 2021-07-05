//==============================================================================
//
//  LibrarySet.h
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "CodeTypes.h"
#include "LibraryTypes.h"
#include "SysTypes.h"

namespace CodeTools
{
   struct CxxUsageSets;
}

namespace NodeBase
{
   class CliThread;
}

//------------------------------------------------------------------------------

namespace CodeTools
{
//  A collection of library items (directories, files, or variables).
//
class LibrarySet : public LibraryItem
{
   friend class Library;
public:
   //  Deleted to prohibit copying.
   //
   LibrarySet(const LibrarySet& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   LibrarySet& operator=(const LibrarySet& that) = delete;

   //  Prefix for the name of a read-only set.
   //
   static const char ReadOnlyChar;

   //  Prefix for the name of a temporary set.
   //
   static const char TemporaryChar;

   //  Returns a name for a temporary variable.
   //
   static std::string TemporaryName();

   //  Returns true if the set is a temporary result that will not be saved.
   //
   bool IsTemporary() const;

   //  Returns the items in the set.
   //
   const LibItemSet& Items() const { return items_; }
   LibItemSet& Items() { return items_; }

   //  Returns the type of set.  Must be overridden by subclasses.
   //
   virtual LibSetType GetType() const;

   //  Returns 0 after checking code files in the set for conformance to
   //  C++ coding guidelines.  If STREAM is not nullptr, produces a report
   //  that contains line counts and warnings.
   //
   virtual NodeBase::word Check(NodeBase::CliThread& cli,
      std::ostream* stream, std::string& expl) const;

   //  On success, returns 0 and updates RESULT with the number of items
   //  in the set.  Returns another value on failure and updates RESULT
   //  with an explanation.
   //
   virtual NodeBase::word Count(std::string& result) const;

   //  On success, returns 0 and updates RESULT with the number of lines
   //  of code in the set.  Returns another value on failure and updates
   //  RESULT with an explanation.
   //
   virtual NodeBase::word Countlines(std::string& result) const;

   //  Returns 0 after fixing warnings detected by Check() in the set,
   //  using OPTS.  Returns another value on failure and updates EXPL with
   //  an explanation.
   //
   virtual NodeBase::word Fix(NodeBase::CliThread& cli,
      FixOptions& opts, std::string& expl) const;

   //  On success, returns 0 after reformatting the file.  Returns another
   //  value on failure and updates EXPL with an explanation.
   //
   virtual NodeBase::word Format(std::string& expl) const;

   //  On success, returns 0 after parsing items in the set.  EXPL describes
   //  the outcome.  The first character in TRACE indicates whether to create
   //  a parse file never ('n'), on failures only ('f'), or always ('a').
   //  The second character indicates whether to create an execution file
   //  never ('n') or always ('a').
   //
   virtual NodeBase::word Parse
      (std::string& expl, const std::string& opts) const;

   //  On success, returns 0 and updates STREAM with lines in the set that
   //  match PATTERN.  Returns another value on failure and updates EXPL
   //  with an explanation.
   //
   virtual NodeBase::word Scan(std::ostream& stream,
      const std::string& pattern, std::string& expl) const;

   //  On success, returns 0 and updates STREAM with the build order of
   //  the set.  Returns another value on failure and updates EXPL with
   //  an explanation.
   //
   virtual NodeBase::word Sort(std::ostream& stream, std::string& expl) const;

   //  Returns the build order of the set.
   //
   virtual BuildOrder SortInBuildOrder() const;

   //  Operators.  The default implementations invoke OpError (see below)
   //  and must be overridden by a subclass that supports the operator.
   //
   virtual LibrarySet* Assign(LibrarySet* that);
   virtual LibrarySet* Intersection(const LibrarySet* that) const;
   virtual LibrarySet* Difference(const LibrarySet* that) const;
   virtual LibrarySet* Union(const LibrarySet* that) const;
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
   virtual LibrarySet* DeclaredBy() const;
   virtual LibrarySet* Declarers() const;
   virtual LibrarySet* Definitions() const;
   virtual LibrarySet* ReferencedBy() const;
   virtual LibrarySet* Referencers() const;

   //  On success, returns 0 and updates STREAM with a list of the items in
   //  the set, one per line.  Returns another value on failure.
   //
   NodeBase::word List(std::ostream& stream) const;

   //  On success, returns 0 and updates RESULT with the items in the set,
   //  separated by commas.  Returns another value on failure and updates
   //  RESULT with an explanation.
   //
   NodeBase::word Show(std::string& result) const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to return the item's name.
   //
   const std::string& Name() const override { return name_; }
protected:
   //  Creates a set that with the identifier NAME.  If NAME is not prefixed
   //  by TemporaryChar, it is added to the set of library variables.  If it
   //  is prefixed by ReadOnlyChar, it is treated as read-only.  Protected
   //  because this class is virtual.
   //
   explicit LibrarySet(const std::string& name);

   //  Removes the set from the queue of sets.  Protected to restrict deletion.
   //  Virtual to allow subclassing.
   //
   virtual ~LibrarySet();

   //  Creates a set that contains ITEMS and is identified by NAME.  The default
   //  implementation invokes OpError (see below) and must be overridden by each
   //  subclass.
   //
   virtual LibrarySet* Create
      (const std::string& name, const LibItemSet* items) const;

   //  Reports COUNT items in RESULT and returns 0.
   //
   static NodeBase::word Counted(std::string& result, const size_t count);
private:
   //  Returns 0 if this set can be assigned to a variable.  Returns another
   //  value and updates EXPL with an explanation if it cannot be assigned.
   //
   virtual NodeBase::word PreAssign(std::string& expl) const;

   //  Copies the items in USAGES into the set.
   //
   virtual void CopyUsages(const CxxUsageSets& usages);

   //  Update STRINGS with a string for each item in the set.  The strings
   //  will either be displayed one per line (VERBOSE is true) or separated
   //  by commas (VERBOSE is false).
   //
   virtual void to_str(stringVector& strings, bool verbose) const = 0;

   //  Deletes the set if it is a temporary result.
   //
   void Release();

   //  Returns true if the set is read-only for CLI commands.
   //
   bool IsReadOnly() const;

   //  If RESULT is not empty, deletes a presumed trailing ", ", else sets
   //  RESULT to indicate that nothing was found.  Returns 0.
   //
   static NodeBase::word Shown(std::string& result);

   //  Update EXPL to indicate that a function is not implemented by the type
   //  of set in question.
   //
   NodeBase::word NotImplemented(std::string& expl) const;

   //  Returns a string to indicate that a function is not implemented by the
   //  type of set in question.
   //
   std::string NotApplicable() const;

   //  Generates a log and returns nullptr.
   //
   LibrarySet* OpError(NodeBase::fixed_string op) const;

   //  The set's name.
   //
   std::string name_;

   //  The set of items.
   //
   LibItemSet items_;

   //  Set if this set is a temporary variable.
   //
   bool temp_;

   //  Sequence number for generating names for temporary variables.
   //
   static uint32_t SeqNo_;
};
}
#endif
