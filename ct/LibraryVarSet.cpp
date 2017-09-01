//==============================================================================
//
//  LibraryVarSet.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "LibraryVarSet.h"
#include "Debug.h"
#include "Library.h"
#include "Q2Way.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::string;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
fn_name LibraryVarSet_ctor = "LibraryVarSet.ctor";

LibraryVarSet::LibraryVarSet(const string& name) : LibrarySet(name)
{
   Debug::ft(LibraryVarSet_ctor);
}

//------------------------------------------------------------------------------

fn_name LibraryVarSet_dtor = "LibraryVarSet.dtor";

LibraryVarSet::~LibraryVarSet()
{
   Debug::ft(LibraryVarSet_dtor);
}

//------------------------------------------------------------------------------

fn_name LibraryVarSet_Count = "LibraryVarSet.Count";

word LibraryVarSet::Count(string& result) const
{
   Debug::ft(LibraryVarSet_Count);

   auto size = Singleton< Library >::Instance()->Variables().Size();
   return Counted(result, &size);
}

//------------------------------------------------------------------------------

fn_name LibraryVarSet_Show = "LibraryVarSet.Show";

word LibraryVarSet::Show(string& result) const
{
   Debug::ft(LibraryVarSet_Show);

   auto& vars = Singleton< Library >::Instance()->Variables();

   for(auto v = vars.First(); v != nullptr; vars.Next(v))
   {
      if(!v->IsTemporary()) result = result + v->Name() + ", ";
   }

   return Shown(result);
}
}