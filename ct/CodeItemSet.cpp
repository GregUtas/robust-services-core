//==============================================================================
//
//  CodeItemSet.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "CodeItemSet.h"
#include <set>
#include <vector>
#include "CodeDir.h"
#include "CodeDirSet.h"
#include "CodeFile.h"
#include "CodeFileSet.h"
#include "Cxx.h"
#include "CxxNamed.h"
#include "CxxScope.h"
#include "Debug.h"

using namespace NodeBase;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
CodeItemSet::CodeItemSet(const string& name, const LibItemSet* items) :
   CodeSet(name, items)
{
   Debug::ft("CodeItemSet.ctor");
}

//------------------------------------------------------------------------------

CodeItemSet::~CodeItemSet()
{
   Debug::ftnt("CodeItemSet.dtor");
}

//------------------------------------------------------------------------------

void CodeItemSet::CopyItems(const CxxNamedSet& items)
{
   auto& itemSet = Items();

   for(auto i = items.cbegin(); i != items.cend(); ++i)
   {
      itemSet.insert(*i);
   }
}

//------------------------------------------------------------------------------

void CodeItemSet::CopyUsages(const CxxUsageSets& usages)
{
   CopyItems(usages.bases);
   CopyItems(usages.directs);
   CopyItems(usages.forwards);
   CopyItems(usages.friends);
   CopyItems(usages.inherits);
}

//------------------------------------------------------------------------------

LibrarySet* CodeItemSet::Create
   (const string& name, const LibItemSet* items) const
{
   Debug::ft("CodeItemSet.Create");

   return new CodeItemSet(name, items);
}

//------------------------------------------------------------------------------

LibrarySet* CodeItemSet::DeclaredBy() const
{
   Debug::ft("CodeItemSet.DeclaredBy");

   auto& itemSet = Items();
   auto result = new CodeItemSet(TemporaryName(), nullptr);
   auto& declSet = result->Items();

   for(auto i = itemSet.cbegin(); i != itemSet.cend(); ++i)
   {
      std::set< CxxNamed* > decls;
      (*i)->GetDecls(decls);

      for(auto d = decls.cbegin(); d != decls.cend(); ++d)
      {
         declSet.insert(*d);
      }
   }

   return result;
}

//------------------------------------------------------------------------------

LibrarySet* CodeItemSet::Declarers() const
{
   Debug::ft("CodeItemSet.Declarers");

   auto& itemSet = Items();
   auto result = new CodeFileSet(TemporaryName(), nullptr);
   auto& declSet = result->Items();

   for(auto i = itemSet.cbegin(); i != itemSet.cend(); ++i)
   {
      auto item = static_cast< CxxNamed* >(*i);
      auto scope = item->GetScope();

      if(scope->Type() == Cxx::Namespace)
      {
         scope = item->GetFile()->FindNamespaceDefn(item);
      }

      if(scope != nullptr) declSet.insert(scope);
   }

   return result;
}

//------------------------------------------------------------------------------

LibrarySet* CodeItemSet::Definitions() const
{
   Debug::ft("CodeItemSet.Definitions");

   auto& itemSet = Items();
   auto result = new CodeItemSet(TemporaryName(), nullptr);
   auto& defSet = result->Items();

   for(auto i = itemSet.cbegin(); i != itemSet.cend(); ++i)
   {
      auto item = static_cast< CxxNamed* >(*i);
      auto mate = item->GetMate();

      if(mate != nullptr)
      {
         auto file = item->GetDefnFile();
         if(mate->GetFile() == file)
            defSet.insert(mate);
         else
            defSet.insert(item);
      }
   }

   return result;
}

//------------------------------------------------------------------------------

LibrarySet* CodeItemSet::Directories() const
{
   Debug::ft("CodeItemSet.Directories");

   auto& itemSet = Items();
   auto result = new CodeDirSet(TemporaryName(), nullptr);
   auto& dirSet = result->Items();

   for(auto i = itemSet.cbegin(); i != itemSet.cend(); ++i)
   {
      auto item = static_cast< CxxNamed* >(*i);
      auto file = item->GetFile();
      if(file == nullptr) continue;
      auto dir = file->Dir();
      if(dir != nullptr) dirSet.insert(dir);
   }

   return result;
}

//------------------------------------------------------------------------------

LibrarySet* CodeItemSet::Files() const
{
   Debug::ft("CodeItemSet.Files");

   auto& itemSet = Items();
   auto result = new CodeFileSet(TemporaryName(), nullptr);
   auto& fileSet = result->Items();

   for(auto i = itemSet.cbegin(); i != itemSet.cend(); ++i)
   {
      auto item = static_cast< CxxNamed* >(*i);
      auto file = item->GetFile();
      if(file != nullptr) fileSet.insert(file);
   }

   return result;
}

//------------------------------------------------------------------------------

LibrarySet* CodeItemSet::ReferencedBy() const
{
   Debug::ft("CodeItemSet.ReferencedBy");

   auto& itemSet = Items();
   auto result = new CodeItemSet(TemporaryName(), nullptr);

   for(auto i = itemSet.cbegin(); i != itemSet.cend(); ++i)
   {
      auto item = static_cast< CxxScoped* >(*i);
      CxxUsageSets usages;
      item->GetUsages(*item->GetFile(), usages);
      result->CopyUsages(usages);
   }

   return result;
}

//------------------------------------------------------------------------------

LibrarySet* CodeItemSet::Referencers() const
{
   Debug::ft("CodeItemSet.Referencers");

   auto& itemSet = Items();
   auto result = new CodeFileSet(TemporaryName(), nullptr);
   auto& refSet = result->Items();

   for(auto i = itemSet.cbegin(); i != itemSet.cend(); ++i)
   {
      auto item = static_cast< CxxScoped* >(*i);
      auto& xref = item->Xref();

      for(auto r = xref.cbegin(); r != xref.cend(); ++r)
      {
         refSet.insert(*r);
      }
   }

   return result;
}

//------------------------------------------------------------------------------

void CodeItemSet::to_str(stringVector& strings, bool verbose) const
{
   Debug::ft("CodeItemSet.to_str");

   auto& itemSet = Items();

   for(auto i = itemSet.cbegin(); i != itemSet.cend(); ++i)
   {
      auto item = static_cast< CxxNamed* >(*i);
      strings.push_back(item->to_str());
   }
}
}
