//==============================================================================
//
//  CodeDir.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CodeDir.h"
#include <ostream>
#include "Algorithms.h"
#include "CodeFile.h"
#include "CxxString.h"
#include "Debug.h"
#include "Library.h"
#include "Registry.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------
//
namespace CodeTools
{
fn_name CodeDir_ctor = "CodeDir.ctor";

CodeDir::CodeDir(const string& name, const string& path) : LibraryItem(name),
   path_(path)
{
   Debug::ft(CodeDir_ctor);
}

//------------------------------------------------------------------------------

fn_name CodeDir_dtor = "CodeDir.dtor";

CodeDir::~CodeDir()
{
   Debug::ft(CodeDir_dtor);
}

//------------------------------------------------------------------------------

ptrdiff_t CodeDir::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const CodeDir* >(&local);
   return ptrdiff(&fake->did_, fake);
}

//------------------------------------------------------------------------------

fn_name CodeDir_CppCount = "CodeDir.CppCount";

size_t CodeDir::CppCount() const
{
   Debug::ft(CodeDir_CppCount);

   size_t count = 0;

   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = files.First(); f != nullptr; files.Next(f))
   {
      if((f->Dir() == this) && f->IsCpp()) ++count;
   }

   return count;
}

//------------------------------------------------------------------------------

void CodeDir::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   LibraryItem::Display(stream, prefix, options);

   stream << prefix << "did  : " << did_.to_str() << CRLF;
   stream << prefix << "path : " << path_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name CodeDir_HeaderCount = "CodeDir.HeaderCount";

size_t CodeDir::HeaderCount() const
{
   Debug::ft(CodeDir_HeaderCount);

   size_t count = 0;

   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = files.First(); f != nullptr; files.Next(f))
   {
      if((f->Dir() == this) && f->IsHeader()) ++count;
   }

   return count;
}

//------------------------------------------------------------------------------

bool CodeDir::IsSubsDir() const
{
   return PathIncludes(path_, Library::SubsDir);
}
}
