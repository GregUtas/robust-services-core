//==============================================================================
//
//  LibrarySet.cpp
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
#include "LibrarySet.h"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <sstream>
#include "Debug.h"
#include "Formatters.h"
#include "Library.h"
#include "NbCliParms.h"
#include "Singleton.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Prefix for the name of a read-only set.
//
static const char ReadOnlyChar = '$';

//  Sequence number for generating names for temporary variables.
//
static uint32_t SeqNo_ = 0;

//------------------------------------------------------------------------------
//
//  If RESULT is not empty, deletes a presumed trailing ", ", else sets
//  RESULT to indicate that nothing was found.  Returns 0.
//
static word Shown(string& result)
{
   Debug::ft("CodeTools.Shown");

   if(result.rfind(", ") == result.size() - 2)
      result.erase(result.size() - 2, 2);
   else if(result.empty())
      result = EmptySet;
   return 0;
}

//==============================================================================

const char LibrarySet::TemporaryChar = '%';

//------------------------------------------------------------------------------

LibrarySet::LibrarySet(const string& name) :
   name_(name),
   temp_(false)
{
   Debug::ft("LibrarySet.ctor");

   if(name_.front() == TemporaryChar)
   {
      temp_ = true;
      name_.erase(0, 1);
   }

   Singleton< Library >::Instance()->AddVar(*this);
}

//------------------------------------------------------------------------------

LibrarySet::~LibrarySet()
{
   Debug::ftnt("LibrarySet.dtor");

   Singleton< Library >::Extant()->EraseVar(this);
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::AffectedBy() const
{
   return OpError("ab");
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Affecters() const
{
   return OpError("as");
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Assign(LibrarySet* that)
{
   OpError("assign");
   return nullptr;
}

//------------------------------------------------------------------------------

word LibrarySet::Check(CliThread& cli, ostream* stream, string& expl) const
{
   Debug::ft("LibrarySet.Check");

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::CommonAffecters() const
{
   return OpError("ca");
}

//------------------------------------------------------------------------------

word LibrarySet::Count(string& result) const
{
   Debug::ft("LibrarySet.Count");

   return NotImplemented(result);
}

//------------------------------------------------------------------------------

word LibrarySet::Counted(string& result, const size_t count)
{
   Debug::ft("LibrarySet.Counted");

   result = "Count: ";

   if(count != 0)
      result += std::to_string(count);
   else
      result += EmptySet;

   return 0;
}

//------------------------------------------------------------------------------

word LibrarySet::Countlines(string& result) const
{
   Debug::ft("LibrarySet.Countlines");

   return NotImplemented(result);
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Create
   (const string& name, const LibItemSet* items) const
{
   return OpError("create");
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::DeclaredBy() const
{
   return OpError("db");
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Declarers() const
{
   return OpError("ds");
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Definitions() const
{
   return OpError("df");
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Difference(const LibrarySet* that) const
{
   return OpError("-");
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Directories() const
{
   return OpError("d");
}

//------------------------------------------------------------------------------

void LibrarySet::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   LibraryItem::Display(stream, prefix, options);

   auto indent = prefix + spaces(2);

   stream << prefix << "temp : " << temp_ << CRLF;
   stream << prefix << "items (" << items_.size() << ") : " << CRLF;

   for(auto i = items_.cbegin(); i != items_.cend(); ++i)
   {
      stream << indent << (*i)->Name() << '(' << *i << ')' << CRLF;
   }
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::FileName(const LibrarySet* that) const
{
   return OpError("fn");
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Files() const
{
   return OpError("f");
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::FileType(const LibrarySet* that) const
{
   return OpError("ft");
}

//------------------------------------------------------------------------------

word LibrarySet::Fix(CliThread& cli, FixOptions& opts, string& expl) const
{
   Debug::ft("LibrarySet.Fix");

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

word LibrarySet::Format(string& expl) const
{
   Debug::ft("LibrarySet.Format");

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::FoundIn(const LibrarySet* that) const
{
   return OpError("in");
}

//------------------------------------------------------------------------------

fn_name LibrarySet_GetType = "LibrarySet.GetType";

LibSetType LibrarySet::GetType() const
{
   Debug::ft(LibrarySet_GetType);

   Debug::SwLog(LibrarySet_GetType, strOver(this), 0);
   return ERR_SET;
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Implements() const
{
   return OpError("im");
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Intersection(const LibrarySet* that) const
{
   return OpError("&");
}

//------------------------------------------------------------------------------

bool LibrarySet::IsReadOnly() const
{
   return (Name().front() == ReadOnlyChar);
}

//------------------------------------------------------------------------------

bool LibrarySet::IsTemporary() const
{
   return temp_;
}

//------------------------------------------------------------------------------

word LibrarySet::List(ostream& stream) const
{
   Debug::ft("LibrarySet.List");

   stringVector strings;
   to_str(strings, true);

   if(strings.empty())
   {
      stream << spaces(2) << EmptySet << CRLF;
      return 0;
   }

   std::sort(strings.begin(), strings.end(), IsSortedAlphabetically);

   for(auto i = strings.cbegin(); i != strings.cend(); ++i)
   {
      stream << spaces(2) << *i << CRLF;
   }

   return 0;
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::MatchString(const LibrarySet* that) const
{
   return OpError("ms");
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::NeededBy() const
{
   return OpError("nb");
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Needers() const
{
   return OpError("ns");
}

//------------------------------------------------------------------------------

string LibrarySet::NotApplicable() const
{
   std::ostringstream stream;
   stream << "This function is not implemented for a " << GetType();
   auto errstr = stream.str();
   return errstr;
}

//------------------------------------------------------------------------------

word LibrarySet::NotImplemented(string& expl) const
{
   std::ostringstream stream;
   stream << "This command is not implemented for a " << GetType();
   expl = stream.str();
   return -3;
}

//------------------------------------------------------------------------------

fn_name LibrarySet_OpError = "LibrarySet.OpError";

LibrarySet* LibrarySet::OpError(fixed_string op) const
{
   std::ostringstream stream;
   stream << "Operator " << op << " is invalid for set type " << GetType();
   Debug::SwLog(LibrarySet_OpError, stream.str(), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

word LibrarySet::Parse(string& expl, const string& opts) const
{
   Debug::ft("LibrarySet.Parse");

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

word LibrarySet::PreAssign(string& expl) const
{
   Debug::ft("LibrarySet.PreAssign");

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::ReferencedBy() const
{
   return OpError("rb");
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Referencers() const
{
   return OpError("rs");
}

//------------------------------------------------------------------------------

void LibrarySet::Release()
{
   Debug::ftnt("LibrarySet.Release");

   if(!IsTemporary()) return;
   delete this;
}

//------------------------------------------------------------------------------

word LibrarySet::Scan
   (ostream& stream, const string& pattern, string& expl) const
{
   Debug::ft("LibrarySet.Scan");

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

word LibrarySet::Show(string& result) const
{
   Debug::ft("LibrarySet.Show");

   stringVector strings;
   to_str(strings, false);

   if(strings.empty())
   {
      result = EmptySet;
      return 0;
   }

   std::sort(strings.begin(), strings.end(), IsSortedAlphabetically);

   for(auto i = strings.cbegin(); i != strings.cend(); ++i)
   {
      result += *i + ", ";
   }

   return Shown(result);
}

//------------------------------------------------------------------------------

word LibrarySet::Sort(ostream& stream, string& expl) const
{
   Debug::ft("LibrarySet.Sort");

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

fn_name LibrarySet_SortInBuildOrder = "LibrarySet.SortInBuildOrder";

BuildOrder LibrarySet::SortInBuildOrder() const
{
   Debug::ft(LibrarySet_SortInBuildOrder);

   BuildOrder order;
   auto errstr = NotApplicable();
   Debug::SwLog(LibrarySet_SortInBuildOrder, errstr, GetType());
   return order;
}

//------------------------------------------------------------------------------

string LibrarySet::TemporaryName()
{
   Debug::ft("LibrarySet.TemporaryName");

   string name = "%temp";
   name += std::to_string(SeqNo_++);
   return name;
}

//------------------------------------------------------------------------------

fn_name LibrarySet_to_str = "LibrarySet.to_str";

void LibrarySet::to_str(stringVector& strings, bool verbose) const
{
   Debug::ft(LibrarySet_to_str);

   Debug::SwLog(LibrarySet_to_str, strOver(this), 0);
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Union(const LibrarySet* that) const
{
   return OpError("|");
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::UsedBy(bool self) const
{
   return OpError("ub");
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Users(bool self) const
{
   return OpError("us");
}
}
