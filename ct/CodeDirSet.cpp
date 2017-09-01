//==============================================================================
//
//  CodeDirSet.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CodeDirSet.h"
#include <ostream>
#include <set>
#include "CodeDir.h"
#include "CodeFile.h"
#include "CodeFileSet.h"
#include "Debug.h"
#include "Formatters.h"
#include "Library.h"
#include "NbCliParms.h"
#include "Registry.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
fn_name CodeDirSet_ctor = "CodeDirSet.ctor";

CodeDirSet::CodeDirSet(const string& name, SetOfIds* set) : CodeSet(name, set)
{
   Debug::ft(CodeDirSet_ctor);
}

//------------------------------------------------------------------------------

fn_name CodeDirSet_dtor = "CodeDirSet.dtor";

CodeDirSet::~CodeDirSet()
{
   Debug::ft(CodeDirSet_dtor);
}

//------------------------------------------------------------------------------

fn_name CodeDirSet_Create = "CodeDirSet.Create";

LibrarySet* CodeDirSet::Create(const string& name, SetOfIds* set) const
{
   Debug::ft(CodeDirSet_Create);

   return new CodeDirSet(name, set);
}

//------------------------------------------------------------------------------

fn_name CodeDirSet_Files = "CodeDirSet.Files";

LibrarySet* CodeDirSet::Files() const
{
   Debug::ft(CodeDirSet_Files);

   //  Iterate over all code files to find those whose directory is in DIRSET.
   //
   auto& dirSet = Set();
   auto result = new CodeFileSet(TemporaryName(), nullptr);
   auto& fileSet = result->Set();
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = files.First(); f != nullptr; files.Next(f))
   {
      auto d = f->Dir();

      if(d != nullptr)
      {
         SetOfIds::const_iterator it = dirSet.find(d->Did());

         if(it != dirSet.cend())
         {
            fileSet.insert(f->Fid());
         }
      }
   }

   return result;
}

//------------------------------------------------------------------------------

fn_name CodeDirSet_List = "CodeDirSet.List";

word CodeDirSet::List(ostream& stream, string& expl) const
{
   Debug::ft(CodeDirSet_List);

   auto& dirSet = Set();

   if(dirSet.empty())
   {
      stream << spaces(2) << EmptySet << CRLF;
      return 0;
   }

   auto& dirs = Singleton< Library >::Instance()->Directories();

   for(auto d = dirSet.cbegin(); d != dirSet.cend(); ++d)
   {
      stream << spaces(2) << dirs.At(*d)->Path() << CRLF;
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name CodeDirSet_Show = "CodeDirSet.Show";

word CodeDirSet::Show(string& result) const
{
   Debug::ft(CodeDirSet_Show);

   auto& dirSet = Set();
   auto& dirs = Singleton< Library >::Instance()->Directories();

   for(auto d = dirSet.cbegin(); d != dirSet.cend(); ++d)
   {
      result = result + dirs.At(*d)->Name() + ", ";
   }

   return Shown(result);
}
}
