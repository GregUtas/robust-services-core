//==============================================================================
//
//  LibrarySet.cpp
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
#include "LibrarySet.h"
#include <ostream>
#include "Algorithms.h"
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
const char LibrarySet::ReadOnlyChar = '$';
const char LibrarySet::TemporaryChar = '%';
uint8_t LibrarySet::SeqNo_ = 0;

//------------------------------------------------------------------------------

fn_name LibrarySet_ctor = "LibrarySet.ctor";

LibrarySet::LibrarySet(const string& name) : LibraryItem(name),
   temp_(false)
{
   Debug::ft(LibrarySet_ctor);

   auto s = AccessName();

   if(name.front() == TemporaryChar)
   {
      temp_ = true;
      s->erase(0, 1);
   }

   Singleton< Library >::Instance()->AddVar(*this);
}

//------------------------------------------------------------------------------

fn_name LibrarySet_dtor = "LibrarySet.dtor";

LibrarySet::~LibrarySet()
{
   Debug::ft(LibrarySet_dtor);

   Singleton< Library >::Instance()->EraseVar(*this);
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::AffectedBy() const
{
   return OpError();
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Affecters() const
{
   return OpError();
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Assign(LibrarySet* rhs)
{
   return OpError();
}

//------------------------------------------------------------------------------

fn_name LibrarySet_Check = "LibrarySet.Check";

word LibrarySet::Check(ostream* stream, string& expl) const
{
   Debug::ft(LibrarySet_Check);

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::CommonAffecters() const
{
   return OpError();
}

//------------------------------------------------------------------------------

fn_name LibrarySet_Count = "LibrarySet.Count";

word LibrarySet::Count(string& result) const
{
   Debug::ft(LibrarySet_Count);

   return NotImplemented(result);
}

//------------------------------------------------------------------------------

fn_name LibrarySet_Counted = "LibrarySet.Counted";

word LibrarySet::Counted(string& result, const size_t* count)
{
   Debug::ft(LibrarySet_Counted);

   result = "Count: ";

   if(count != nullptr)
      result += std::to_string(*count);
   else
      result += EmptySet;

   return 0;
}

//------------------------------------------------------------------------------

fn_name LibrarySet_Countlines = "LibrarySet.Countlines";

word LibrarySet::Countlines(string& result) const
{
   Debug::ft(LibrarySet_Countlines);

   return NotImplemented(result);
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Create(const string& name, SetOfIds* set) const
{
   return OpError();
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Difference(const LibrarySet* rhs) const
{
   return OpError();
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Directories() const
{
   return OpError();
}

//------------------------------------------------------------------------------

void LibrarySet::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   LibraryItem::Display(stream, prefix, options);

   stream << prefix << "link : " << CRLF;
   link_.Display(stream, prefix + spaces(2));
   stream << prefix << "temp : " << temp_ << CRLF;
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::FileName(const LibrarySet* that) const
{
   return OpError();
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Files() const
{
   return OpError();
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::FileType(const LibrarySet* that) const
{
   return OpError();
}

//------------------------------------------------------------------------------

fn_name LibrarySet_Fix = "LibrarySet.Fix";

word LibrarySet::Fix(CliThread& cli, const FixOptions& opts, string& expl) const
{
   Debug::ft(LibrarySet_Fix);

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

fn_name LibrarySet_Format = "LibrarySet.Format";

word LibrarySet::Format(string& expl) const
{
   Debug::ft(LibrarySet_Format);

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::FoundIn(const LibrarySet* that) const
{
   return OpError();
}

//------------------------------------------------------------------------------

fn_name LibrarySet_GetType = "LibrarySet.GetType";

LibSetType LibrarySet::GetType() const
{
   Debug::ft(LibrarySet_GetType);

   Debug::SwLog(LibrarySet_GetType, 0, 0);
   return ERR_SET;
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Implements() const
{
   return OpError();
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Intersection(const LibrarySet* rhs) const
{
   return OpError();
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

ptrdiff_t LibrarySet::LinkDiff()
{
   int local;
   auto fake = reinterpret_cast< const LibrarySet* >(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

fn_name LibrarySet_List = "LibrarySet.List";

word LibrarySet::List(ostream& stream, string& expl) const
{
   Debug::ft(LibrarySet_List);

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::MatchString(const LibrarySet* that) const
{
   return OpError();
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::NeededBy() const
{
   return OpError();
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Needers() const
{
   return OpError();
}

//------------------------------------------------------------------------------

fn_name LibrarySet_NotImplemented = "LibrarySet.NotImplemented";

word LibrarySet::NotImplemented(string& expl) const
{
   switch(GetType())
   {
   case DIR_SET:
      expl = "This command is not implemented for directories.";
      break;
   case FILE_SET:
      expl = "This command is not implemented for code files.";
      break;
   case VAR_SET:
      expl = "This command is not implemented for variables.";
      break;
   default:
      Debug::SwLog(LibrarySet_NotImplemented, GetType(), 0);
      expl = "Internal error.";
      return -8;
   }

   return -3;
}

//------------------------------------------------------------------------------

fn_name LibrarySet_OpError = "LibrarySet.OpError";

LibrarySet* LibrarySet::OpError() const
{
   Debug::SwLog(LibrarySet_OpError, GetType(), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name LibrarySet_Parse = "LibrarySet.Parse";

word LibrarySet::Parse(string& expl, const string& opts) const
{
   Debug::ft(LibrarySet_Parse);

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

fn_name LibrarySet_PreAssign = "LibrarySet.PreAssign";

word LibrarySet::PreAssign(string& expl) const
{
   Debug::ft(LibrarySet_PreAssign);

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

fn_name LibrarySet_Release = "LibrarySet.Release";

void LibrarySet::Release()
{
   Debug::ft(LibrarySet_Release);

   if(!IsTemporary()) return;
   delete this;
}

//------------------------------------------------------------------------------

fn_name LibrarySet_Scan = "LibrarySet.Scan";

word LibrarySet::Scan
   (ostream& stream, const string& pattern, string& expl) const
{
   Debug::ft(LibrarySet_Scan);

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

fn_name LibrarySet_Show = "LibrarySet.Show";

word LibrarySet::Show(string& result) const
{
   Debug::ft(LibrarySet_Show);

   return NotImplemented(result);
}

//------------------------------------------------------------------------------

fn_name LibrarySet_Shown = "LibrarySet.Shown";

word LibrarySet::Shown(string& result)
{
   Debug::ft(LibrarySet_Shown);

   if(result.rfind(", ") != string::npos)
      result.erase(result.size() - 2, 2);
   else if(result.empty())
      result = EmptySet;
   return 0;
}

//------------------------------------------------------------------------------

fn_name LibrarySet_Sort = "LibrarySet.Sort";

word LibrarySet::Sort(ostream& stream, string& expl) const
{
   Debug::ft(LibrarySet_Sort);

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

fn_name LibrarySet_TemporaryName = "LibrarySet.TemporaryName";

string LibrarySet::TemporaryName()
{
   Debug::ft(LibrarySet_TemporaryName);

   string name = "%temp";
   name += std::to_string(int(SeqNo_));
   SeqNo_++;
   return name;
}

//------------------------------------------------------------------------------

fn_name LibrarySet_Trim = "LibrarySet.Trim";

word LibrarySet::Trim(ostream& stream, string& expl) const
{
   Debug::ft(LibrarySet_Trim);

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Union(const LibrarySet* rhs) const
{
   return OpError();
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::UsedBy(bool self) const
{
   return OpError();
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Users(bool self) const
{
   return OpError();
}
}
