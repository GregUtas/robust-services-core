//==============================================================================
//
//  LibrarySet.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "LibrarySet.h"
#include <sstream>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "Library.h"
#include "NbCliParms.h"
#include "Singleton.h"

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

word LibrarySet::Check(ostream& stream, string& expl) const
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

   std::ostringstream stream;

   stream << "Count: ";

   if(count != nullptr)
      stream << *count;
   else
      stream << EmptySet;

   result = stream.str();
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

LibrarySet* LibrarySet::Difference(const LibrarySet* that) const
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

   Debug::SwErr(LibrarySet_GetType, 0, 0);
   return ERR_SET;
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Implements() const
{
   return OpError();
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Intersection(const LibrarySet* that) const
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
      Debug::SwErr(LibrarySet_NotImplemented, GetType(), 0);
      expl = "Internal error.";
      return -8;
   }

   return -3;
}

//------------------------------------------------------------------------------

fn_name LibrarySet_OpError = "LibrarySet.OpError";

LibrarySet* LibrarySet::OpError() const
{
   Debug::SwErr(LibrarySet_OpError, GetType(), 0);
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

word LibrarySet::Show(string& list) const
{
   Debug::ft(LibrarySet_Show);

   return NotImplemented(list);
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

   std::ostringstream stream;
   stream << "%temp" << int(SeqNo_);
   SeqNo_++;
   return stream.str();
}

//------------------------------------------------------------------------------

fn_name LibrarySet_Trim = "LibrarySet.Trim";

word LibrarySet::Trim(ostream& stream, string& expl) const
{
   Debug::ft(LibrarySet_Trim);

   return NotImplemented(expl);
}

//------------------------------------------------------------------------------

LibrarySet* LibrarySet::Union(const LibrarySet* that) const
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
