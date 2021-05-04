//==============================================================================
//
//  CxxSymbols.cpp
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
#include "CxxSymbols.h"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <ostream>
#include <set>
#include "CodeFile.h"
#include "Cxx.h"
#include "CxxArea.h"
#include "CxxDirective.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxScope.h"
#include "CxxScoped.h"
#include "CxxToken.h"
#include "Debug.h"
#include "Formatters.h"
#include "Lexer.h"
#include "Singleton.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
const Flags CLASS_MASK = Flags(1 << Cxx::Class);
const Flags DATA_MASK = Flags(1 << Cxx::Data);
const Flags ENUM_MASK = Flags(1 << Cxx::Enum);
const Flags ETOR_MASK = Flags(1 << Cxx::Enumerator);
const Flags FORW_MASK = Flags(1 << Cxx::Forward);
const Flags FRIEND_MASK = Flags(1 << Cxx::Friend);
const Flags FUNC_MASK = Flags(1 << Cxx::Function);
const Flags MACRO_MASK = Flags(1 << Cxx::Macro);
const Flags SPACE_MASK = Flags(1 << Cxx::Namespace);
const Flags TERM_MASK = Flags(1 << Cxx::Terminal);
const Flags TYPE_MASK = Flags(1 << Cxx::Typedef);

const Flags CODE_REFS = Flags(CLASS_MASK | DATA_MASK | ENUM_MASK |
   ETOR_MASK | FORW_MASK | FRIEND_MASK | FUNC_MASK | MACRO_MASK | TYPE_MASK);
const Flags ITEM_REFS = Flags(CLASS_MASK | DATA_MASK | ENUM_MASK | ETOR_MASK |
   FORW_MASK | FRIEND_MASK | FUNC_MASK | MACRO_MASK | SPACE_MASK | TYPE_MASK);
const Flags FRIEND_CLASSES = Flags(CLASS_MASK | FORW_MASK |
   FRIEND_MASK | TYPE_MASK);
const Flags FRIEND_FUNCS = Flags(FRIEND_CLASSES | FUNC_MASK);
const Flags SCOPE_REFS = Flags(CLASS_MASK | ENUM_MASK | SPACE_MASK | TYPE_MASK);
const Flags TARG_REFS = Flags(CLASS_MASK | DATA_MASK | ENUM_MASK | ETOR_MASK |
   FORW_MASK | FRIEND_MASK | TERM_MASK | MACRO_MASK | TYPE_MASK);
const Flags TYPE_REFS = Flags(CLASS_MASK | TERM_MASK | TYPE_MASK);
const Flags TYPESPEC_REFS = Flags(CLASS_MASK | ENUM_MASK |
   FORW_MASK | FRIEND_MASK | TERM_MASK | TYPE_MASK);
const Flags USING_REFS = Flags(CLASS_MASK | DATA_MASK | ENUM_MASK | ETOR_MASK |
   FORW_MASK | FRIEND_MASK | FUNC_MASK | SPACE_MASK | TYPE_MASK);
const Flags VALUE_REFS = Flags(DATA_MASK | ETOR_MASK | MACRO_MASK | TERM_MASK);

//------------------------------------------------------------------------------
//
//  Key-value pairs for hash tables.
//
typedef std::pair< string, Class* > ClassPair;
typedef std::pair< string, Data* > DataPair;
typedef std::pair< string, Enum* > EnumPair;
typedef std::pair< string, Enumerator* > EtorPair;
typedef std::pair< string, Forward* > ForwPair;
typedef std::pair< string, Friend* > FriendPair;
typedef std::pair< string, Function* > FuncPair;
typedef std::pair< string, Macro* > MacroPair;
typedef std::pair< string, Namespace* > SpacePair;
typedef std::pair< string, Terminal* > TermPair;
typedef std::pair< string, Typedef* > TypePair;

//------------------------------------------------------------------------------
//
//  Displays REFS (references to a single item) in STREAM.
//
const word LAST_XREF_START_COLUMN = 122;

void DisplayReferences(ostream& stream, const CxxNamedVector& refs)
{
   if(refs.empty()) return;

   CodeFile* refFile = nullptr;
   auto endline = false;
   word room = LAST_XREF_START_COLUMN;

   for(auto r = refs.cbegin(); r != refs.cend(); ++r)
   {
      if((*r)->IsInTemplateInstance()) continue;

      auto file = (*r)->GetFile();
      if(file == nullptr) continue;

      if(file != refFile)
      {
         if(refFile != nullptr) stream << CRLF;
         auto fn = file->Path(false);
         stream << spaces(6) << fn << ':';
         refFile = file;
         endline = false;
         room = LAST_XREF_START_COLUMN - (fn.size() + 7);
      }

      if(endline)
      {
         stream << CRLF << spaces(8);
         room -= 8;
         endline = false;
      }

      auto line = file->GetLexer().GetLineNum((*r)->GetPos());
      auto num = std::to_string(++line);
      stream << SPACE << num;
      room -= (num.size() + 1);

      if(room < 0)
      {
         endline = true;
         room = LAST_XREF_START_COLUMN;
      }
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

void FilterItems(const std::string& name,
   const SymbolVector& items, SymbolVector& list)
{
   Debug::ft("CodeTools.FilterItems");

   for(auto i = items.cbegin(); i != items.cend(); ++i)
   {
      stringVector fqNames;
      (*i)->GetScopedNames(fqNames, false);

      for(auto fqn = fqNames.begin(); fqn != fqNames.end(); ++fqn)
      {
         if(NameCouldReferTo(*fqn, name) != string::npos) list.push_back(*i);
      }
   }
}

//------------------------------------------------------------------------------

size_t FindNearestItem(const SymbolVector& list)
{
   Debug::ft("CodeTools.FindNearestItem(list)");

   //  Return the match in the nearest scope.
   //
   size_t min = NOT_A_SUBSCOPE;
   size_t idx = SIZE_MAX;
   auto size = list.size();
   auto scope = Context::Scope();

   for(size_t i = 0; i < size; ++i)
   {
      auto dist = scope->ScopeDistance(list[i]->GetScope());

      if(dist < min)
      {
         min = dist;
         idx = i;
      }
   }

   return idx;
}

//------------------------------------------------------------------------------
//
//  Returns the index of the item in LIST that is nearest the context scope,
//  preferring a resolved forward or friend declaration to one that has not
//  been resolved.  Returns SIZE_MAX if none of the items in LIST is in a
//  related scope.
//
size_t FindNearestItem(const SymbolVector& list, ViewVector& views)
{
   Debug::ft("CodeTools.FindNearestItem(views)");

   //  Return the match in the nearest scope, but preferring a resolved
   //  forward or friend declaration to one that is still unresolved.
   //
   size_t min = NOT_A_SUBSCOPE;
   size_t idx = SIZE_MAX;
   auto size = views.size();

   for(size_t i = 0; i < size; ++i)
   {
      auto& item = list[i];
      auto& view = views[i];

      view.resolved = (item->Referent() != nullptr);

      if(view.distance < min)
      {
         min = view.distance;
         idx = i;
      }
      else if(view.resolved && (view.distance == min))
      {
         if((idx == SIZE_MAX) || !views[idx].resolved)
         {
            idx = i;
         }
      }
   }

   return idx;
}

//------------------------------------------------------------------------------
//
//  Adds all symbols in TABLE to ITEMS.
//
template< typename T > void GetSymbols
   (const std::unordered_multimap< string, T >& table, CxxScopedVector& items)
{
   for(auto i = table.cbegin(); i != table.cend(); ++i)
   {
      auto item = i->second;
      if(!item->IsInternal()) items.push_back(item);
   }
}

//------------------------------------------------------------------------------

bool IsSortedByName(const CxxScoped* item1, const CxxScoped* item2)
{
   auto file1 = item1->GetFile();
   auto file2 = item2->GetFile();
   if(file1 == nullptr)
   {
      if(file2 != nullptr) return true;
   }
   else if(file2 == nullptr)
   {
      return false;
   }
   else
   {
      auto result = strCompare(file1->Path(false), file2->Path(false));
      if(result < 0) return true;
      if(result > 0) return false;
   }

   auto result = strCompare(item1->XrefName(true), item2->XrefName(true));
   if(result < 0) return true;
   if(result > 0) return false;

   result = strCompare(strClass(item1), strClass(item2));
   if(result < 0) return true;
   if(result > 0) return false;
   return (item1 < item2);
}

//------------------------------------------------------------------------------

bool IsSortedByScope(const CxxScoped* item1, const CxxScoped* item2)
{
   //  The first comparison ignores case, whereas the second one does not.
   //  This yields consistent ordering when two names only differ in case.
   //
   auto result = strCompare(item1->ScopedName(true), item2->ScopedName(true));
   if(result < 0) return true;
   if(result > 0) return false;
   return (item1 < item2);
}

//------------------------------------------------------------------------------

CxxSymbols::CxxSymbols()
{
   Debug::ft("CxxSymbols.ctor");

   CxxStats::Incr(CxxStats::CXX_SYMBOLS);
}

//------------------------------------------------------------------------------

CxxSymbols::~CxxSymbols()
{
   Debug::ftnt("CxxSymbols.dtor");

   CxxStats::Decr(CxxStats::CXX_SYMBOLS);
}

//------------------------------------------------------------------------------

void CxxSymbols::DisplayXref(ostream& stream) const
{
   Debug::ft("CxxSymbols.DisplayXref");

   //  Start by displaying references to namespaces.
   //
   CxxScopedVector namespaces;
   GetSymbols(*spaces_, namespaces);

   if(!namespaces.empty())
   {
      stream << "NAMESPACES:" << CRLF;
      std::sort(namespaces.begin(), namespaces.end(), IsSortedByScope);

      for(auto n = namespaces.begin(); n != namespaces.end(); ++n)
      {
         CxxNamedVector refs;
         auto& xref = (*n)->Xref();

         for(auto r = xref.cbegin(); r != xref.cend(); ++r)
         {
            refs.push_back(*r);
         }

         std::sort(refs.begin(), refs.end(), IsSortedByFilePos);

         auto name = (*n)->XrefName(true);
         if(name.empty()) continue;
         stream << spaces(3) << name << CRLF;
         DisplayReferences(stream, refs);
      }
   }

   //  Make a list of all other items that will appear in the cross-reference.
   //  Sort them by directory-file-name.  A few items (such as #defined names
   //  for the compile) don't appear in a file, so put them under "EXTERNAL".
   //
   CxxScopedVector items;
   GetSymbols(*classes_, items);
   GetSymbols(*data_, items);
   GetSymbols(*enums_, items);
   GetSymbols(*etors_, items);
   GetSymbols(*forws_, items);
   GetSymbols(*friends_, items);
   GetSymbols(*funcs_, items);
   GetSymbols(*macros_, items);
   GetSymbols(*types_, items);
   std::sort(items.begin(), items.end(), IsSortedByName);

   CodeFile* itemFile = (CodeFile*) UINTPTR_MAX;

   for(auto i = items.begin(); i != items.end(); ++i)
   {
      CxxNamedVector refs;
      auto& xref = (*i)->Xref();

      for(auto r = xref.cbegin(); r != xref.cend(); ++r)
      {
         refs.push_back(*r);
      }

      std::sort(refs.begin(), refs.end(), IsSortedByFilePos);

      auto file = (*i)->GetFile();
      if(file != itemFile)
      {
         if(file == nullptr)
         {
            stream << "EXTERNAL:" << CRLF;
         }
         else
         {
            if(itemFile == nullptr) stream << "FILES:" << CRLF;
            stream << file->Path(false) << CRLF;
         }

         itemFile = file;
      }

      auto name = (*i)->XrefName(true);
      if(name.empty()) continue;
      stream << spaces(3) << name;

      if(file != nullptr)
      {
         auto line = file->GetLexer().GetLineNum((*i)->GetPos());
         stream << ": " << ++line;
      }

      stream << " [" << strClass(*i, false) << ']' << CRLF;

      DisplayReferences(stream, refs);
   }
}

//------------------------------------------------------------------------------

void CxxSymbols::EraseClass(const Class* cls)
{
   EraseSymbol(cls, *classes_);
}

//------------------------------------------------------------------------------

void CxxSymbols::EraseData(const Data* data)
{
   EraseSymbol(data, *data_);
}

//------------------------------------------------------------------------------

void CxxSymbols::EraseEnum(const Enum* item)
{
   EraseSymbol(item, *enums_);
}

//------------------------------------------------------------------------------

void CxxSymbols::EraseEtor(const Enumerator* etor)
{
   EraseSymbol(etor, *etors_);
}

//------------------------------------------------------------------------------

void CxxSymbols::EraseForw(const Forward* forw)
{
   EraseSymbol(forw, *forws_);
}

//------------------------------------------------------------------------------

void CxxSymbols::EraseFriend(const Friend* frnd)
{
   EraseSymbol(frnd, *friends_);
}

//------------------------------------------------------------------------------

void CxxSymbols::EraseFunc(const Function* func)
{
   EraseSymbol(func, *funcs_);
}

//------------------------------------------------------------------------------

void CxxSymbols::EraseMacro(const Macro* macro)
{
   EraseSymbol(macro, *macros_);
}

//------------------------------------------------------------------------------

void CxxSymbols::EraseSpace(const Namespace* space)
{
   EraseSymbol(space, *spaces_);
}

//------------------------------------------------------------------------------

void CxxSymbols::EraseTerm(const Terminal* term)
{
   EraseSymbol(term, *terms_);
}

//------------------------------------------------------------------------------

void CxxSymbols::EraseType(const Typedef* type)
{
   EraseSymbol(type, *types_);
}

//------------------------------------------------------------------------------

void CxxSymbols::FindItems
   (const string& name, const Flags& mask, SymbolVector& list) const
{
   Debug::ft("CxxSymbols.FindItems");

   auto key = Normalize(name);

   //  Start by looking for a terminal.
   //
   if(mask.test(Cxx::Terminal))
   {
      ListSymbols(key, *terms_, list);
      if(!list.empty()) return;
   }

   //  NAME wasn't a terminal, so look at other types of symbols.
   //
   SymbolVector items;

   if(mask.test(Cxx::Class)) ListSymbols(key, *classes_, items);
   if(mask.test(Cxx::Data)) ListSymbols(key, *data_, items);
   if(mask.test(Cxx::Enum)) ListSymbols(key, *enums_, items);
   if(mask.test(Cxx::Enumerator)) ListSymbols(key, *etors_, items);
   if(mask.test(Cxx::Macro)) ListMacros(key, items);
   if(mask.test(Cxx::Typedef)) ListSymbols(key, *types_, items);
   if(mask.test(Cxx::Namespace)) ListSymbols(key, *spaces_, items);
   if(mask.test(Cxx::Function)) ListSymbols(key, *funcs_, items);
   if(mask.test(Cxx::Forward)) ListSymbols(key, *forws_, items);

   FilterItems(name, items, list);
   if(!list.empty()) return;

   //  There was no match, so consider friend declarations, which
   //  can double as forward declarations.
   //
   if(mask.test(Cxx::Friend)) ListSymbols(key, *friends_, items);
   FilterItems(name, items, list);
}

//------------------------------------------------------------------------------

fn_name CxxSymbols_FindMacro = "CxxSymbols.FindMacro";

Macro* CxxSymbols::FindMacro(const string& name) const
{
   Debug::ft(CxxSymbols_FindMacro);

   SymbolVector macros;
   ListSymbols(name, *macros_, macros);

   if(macros.empty()) return nullptr;

   if(macros.size() > 1)
   {
      auto expl = name + " has more than one definition";
      Context::SwLog(CxxSymbols_FindMacro, expl, macros.size());
   }

   return static_cast< Macro* >(macros.front());
}

//------------------------------------------------------------------------------

CxxScope* CxxSymbols::FindScope(const CxxScope* scope, string& name) const
{
   Debug::ft("CxxSymbols.FindScope");

   //  Erase any leading scope qualifier, as we plan to use full names that
   //  do not have a leading scope qualifier.  If a leading scope qualifier
   //  is not present, prefix the name of any supplied scope to NAME unless
   //  NAME (redundantly) includes that scope.  Template arguments are left
   //  out because a template instance is in the scope of the class template.
   //
   if(name.compare(0, 2, SCOPE_STR) == 0)
   {
      name.erase(0, 2);
   }
   else if(scope != nullptr)
   {
      auto fqScope = scope->ScopedName(false);

      if(name.find(fqScope) == string::npos)
      {
         name = fqScope + SCOPE_STR + name;
      }
   }

   //  Look for a matching namespace or class.
   //
   auto key = Normalize(name);
   SymbolVector spaces;
   ListSymbols(key, *spaces_, spaces);

   for(auto s = spaces.cbegin(); s != spaces.cend(); ++s)
   {
      if((*s)->ScopedName(false) == name) return static_cast< CxxScope* >(*s);
   }

   SymbolVector classes;
   ListSymbols(key, *classes_, classes);

   for(auto c = classes.cbegin(); c != classes.cend(); ++c)
   {
      if((*c)->ScopedName(false) == name) return static_cast< CxxScope* >(*c);
   }

   //  A full match failed, so look for a partial one.
   //
   for(auto s = spaces.cbegin(); s != spaces.cend(); ++s)
   {
      auto pos = NameCouldReferTo((*s)->ScopedName(false), name);
      if(pos != string::npos) return static_cast< CxxScope* >(*s);
   }

   for(auto c = classes.cbegin(); c != classes.cend(); ++c)
   {
      auto pos = NameCouldReferTo((*c)->ScopedName(false), name);
      if(pos != string::npos) return static_cast< CxxScope* >(*c);
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name CxxSymbols_FindSymbol = "CxxSymbols.FindSymbol";

CxxScoped* CxxSymbols::FindSymbol(CodeFile* file,
   const CxxScope* scope, const string& name, const Flags& mask,
   SymbolView& view, const CxxArea* area) const
{
   Debug::ft(CxxSymbols_FindSymbol);

   SymbolVector list1;
   ViewVector views1;

   FindSymbols(file, scope, name, mask, list1, views1, area);

   auto size = list1.size();

   if(size == 1)
   {
      view = views1.front();
      return list1.front();
   }

   if(size == 0) return nullptr;

   //  There were multiple matches.  The priority scheme is
   //  o declared in the same class
   //  o declared in a base class
   //  o declared in a namespace
   //
   SymbolVector list2;
   ViewVector views2;

   for(size_t i = 0; i < size; ++i)
   {
      if(views1[i].accessibility == Declared)
      {
         list2.push_back(list1[i]);
         views2.push_back(views1[i]);
      }
   }

   if(list2.empty())
   {
      for(size_t i = 0; i < size; ++i)
      {
         if(views1[i].accessibility == Inherited)
         {
            list2.push_back(list1[i]);
            views2.push_back(views1[i]);
         }
      }

      if(list2.empty())
      {
         auto gns = Singleton< CxxRoot >::Instance()->GlobalNamespace();

         for(size_t i = 0; i < size; ++i)
         {
            if(list1[i]->GetSpace() != gns)
            {
               list2.push_back(list1[i]);
               views2.push_back(views1[i]);
            }
         }

         if(list2.empty())
         {
            list2.swap(list1);
            views2.swap(views1);
         }
      }
   }

   size = list2.size();

   if(size > 1)
   {
      auto idx = FindNearestItem(list2, views2);

      if(idx != SIZE_MAX)
      {
         view = views2[idx];
         return list2[idx];
      }

      //  The nearest item could not be determined.  This occurs if NAME is
      //  in the global namespace or an unrelated namespace and NAME isn't
      //  unique.  Here are the current explanations:
      //  o When NAME is used indirectly, it can find multiple forward and
      //    friend declarations for the primary item.
      //  o When NAME is that of a class, it can find the class and its
      //    constructor(s).
      //  o When NAME is a class template or one of its members, it can also
      //    be defined in the template's instantiations.  An instantiation's
      //    class can be detected because it has template arguments.
      //  Go through the extra items, and only generate a log if none of the
      //  above apply.
      //
      auto log = false;

      for(size_t i = 1; i < size; ++i)
      {
         auto& item = list2[i];

         switch(item->Type())
         {
         case Cxx::Forward:
         case Cxx::Friend:
         case Cxx::Function:
            break;

         default:
            log = true;

            for(auto c = item->GetClass(); c != nullptr; c = c->OuterClass())
            {
               if(c->GetTemplateArgs() != nullptr)
               {
                  log = false;
                  break;
               }
            }
         }

         if(log)
         {
            auto expl = name + " has more than one definition";
            Context::SwLog(CxxSymbols_FindSymbol, expl, size);
            break;
         }
      }
   }

   view = views2.front();
   return list2.front();
}

//------------------------------------------------------------------------------

void CxxSymbols::FindSymbols(CodeFile* file, const CxxScope* scope,
   const string& name, const Flags& mask, SymbolVector& list,
   ViewVector& views, const CxxArea* area) const
{
   Debug::ft("CxxSymbols.FindSymbols");

   auto key = Normalize(name);

   //  Start by looking for a terminal.
   //
   if(mask.test(Cxx::Terminal))
   {
      ListSymbols(key, *terms_, list);

      if(!list.empty())
      {
         views.push_back(DeclaredGlobally);
         return;
      }
   }

   //  NAME wasn't a terminal, so look at other types of symbols.
   //
   SymbolVector items;

   if(mask.test(Cxx::Class)) ListSymbols(key, *classes_, items);
   if(mask.test(Cxx::Data)) ListSymbols(key, *data_, items);
   if(mask.test(Cxx::Enum)) ListSymbols(key, *enums_, items);
   if(mask.test(Cxx::Enumerator)) ListSymbols(key, *etors_, items);
   if(mask.test(Cxx::Macro)) ListMacros(key, items);
   if(mask.test(Cxx::Typedef)) ListSymbols(key, *types_, items);
   if(mask.test(Cxx::Namespace)) ListSymbols(key, *spaces_, items);
   if(mask.test(Cxx::Function)) ListSymbols(key, *funcs_, items);
   if(mask.test(Cxx::Forward)) ListSymbols(key, *forws_, items);

   for(auto i = items.cbegin(); i != items.cend(); ++i)
   {
      if((area == nullptr) || ((*i)->IsDefinedIn(area)))
      {
         SymbolView view;

         if((*i)->NameRefersToItem(name, scope, file, view))
         {
            list.push_back(*i);
            views.push_back(view);
         }
      }
   }

   if(!list.empty()) return;

   //  There was no match, so consider friend declarations, which
   //  can double as forward declarations.
   //
   items.clear();
   if(mask.test(Cxx::Friend)) ListSymbols(key, *friends_, items);

   for(auto i = items.cbegin(); i != items.cend(); ++i)
   {
      if((area == nullptr) || ((*i)->IsDefinedIn(area)))
      {
         SymbolView view;

         if((*i)->NameRefersToItem(name, scope, file, view))
         {
            list.push_back(*i);
            views.push_back(view);
         }
      }
   }
}

//------------------------------------------------------------------------------

void CxxSymbols::FindTerminal(const string& name, SymbolVector& list) const
{
   ListSymbols(name, *terms_, list);
}

//------------------------------------------------------------------------------

void CxxSymbols::InsertClass(Class* cls)
{
   classes_->insert(ClassPair(Normalize(cls->Name()), cls));
}

//------------------------------------------------------------------------------

void CxxSymbols::InsertData(Data* data)
{
   data_->insert(DataPair(Normalize(data->Name()), data));
}

//------------------------------------------------------------------------------

void CxxSymbols::InsertEnum(Enum* item)
{
   enums_->insert(EnumPair(Normalize(item->Name()), item));
}

//------------------------------------------------------------------------------

void CxxSymbols::InsertEtor(Enumerator* etor)
{
   etors_->insert(EtorPair(Normalize(etor->Name()), etor));
}

//------------------------------------------------------------------------------

void CxxSymbols::InsertForw(Forward* forw)
{
   forws_->insert(ForwPair(Normalize(forw->Name()), forw));
}

//------------------------------------------------------------------------------

void CxxSymbols::InsertFriend(Friend* frnd)
{
   friends_->insert(FriendPair(Normalize(frnd->Name()), frnd));
}

//------------------------------------------------------------------------------

void CxxSymbols::InsertFunc(Function* func)
{
   funcs_->insert(FuncPair(Normalize(func->Name()), func));
}

//------------------------------------------------------------------------------

void CxxSymbols::InsertMacro(Macro* macro)
{
   macros_->insert(MacroPair(Normalize(macro->Name()), macro));
}

//------------------------------------------------------------------------------

void CxxSymbols::InsertSpace(Namespace* space)
{
   spaces_->insert(SpacePair(Normalize(space->Name()), space));
}

//------------------------------------------------------------------------------

void CxxSymbols::InsertTerm(Terminal* term)
{
   terms_->insert(TermPair(Normalize(term->Name()), term));
}

//------------------------------------------------------------------------------

void CxxSymbols::InsertType(Typedef* type)
{
   types_->insert(TypePair(Normalize(type->Name()), type));
}

//------------------------------------------------------------------------------

bool CxxSymbols::IsUniqueName
   (const CxxScope* scope, const std::string& name) const
{
   Debug::ft("CxxSymbols.IsUniqueName");

   //  This only needs to look for functions.
   //
   size_t count = 0;
   auto key = Normalize(name);
   SymbolVector items;

   ListSymbols(key, *funcs_, items);

   for(auto i = items.cbegin(); i != items.cend(); ++i)
   {
      if((*i)->GetScope() == scope) ++count;
      if(count > 1) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

void CxxSymbols::ListMacros(const string& name, SymbolVector& list) const
{
   Debug::ft("CxxSymbols.ListMacros");

   auto last = macros_->upper_bound(name);

   for(auto i = macros_->lower_bound(name); i != last; ++i)
   {
      if(i->second->IsDefined()) list.push_back(i->second);
   }
}

//------------------------------------------------------------------------------

void CxxSymbols::Shrink() const
{
   //  This cannot shrink its containers.  An unordered multimap does not
   //  support shrink_to_fit, and the strings in each tuple are const.
   //
   size_t ssize = 0;
   size_t vsize = 0;

   vsize += classes_->size() * (sizeof(ClassPair) + 2 * sizeof(ClassPair*));
   vsize += classes_->bucket_count() * (sizeof(ClassPair*) + sizeof(size_t));

   for(auto c = classes_->cbegin(); c != classes_->cend(); ++c)
   {
      ssize += c->first.capacity();
   }

   vsize += data_->size() * (sizeof(DataPair) + 2 * sizeof(DataPair*));
   vsize += data_->bucket_count() * (sizeof(DataPair*) + sizeof(size_t));

   for(auto d = data_->cbegin(); d != data_->cend(); ++d)
   {
      ssize += d->first.capacity();
   }

   vsize += macros_->size() * (sizeof(MacroPair) + 2 * sizeof(MacroPair*));
   vsize += macros_->bucket_count() * (sizeof(MacroPair*) + sizeof(size_t));

   for(auto m = macros_->cbegin(); m != macros_->cend(); ++m)
   {
      ssize += m->first.capacity();
   }

   vsize += enums_->size() * (sizeof(EnumPair) + 2 * sizeof(ClassPair*));
   vsize += enums_->bucket_count() * (sizeof(EnumPair*) + sizeof(size_t));

   for(auto e = enums_->cbegin(); e != enums_->cend(); ++e)
   {
      ssize += e->first.capacity();
   }

   vsize += etors_->size() * (sizeof(EtorPair) + 2 * sizeof(EtorPair*));
   vsize += etors_->bucket_count() * (sizeof(EtorPair*) + sizeof(size_t));

   for(auto e = etors_->cbegin(); e != etors_->cend(); ++e)
   {
      ssize += e->first.capacity();
   }

   vsize += forws_->size() * (sizeof(ForwPair) + 2 * sizeof(ForwPair*));
   vsize += forws_->bucket_count() * (sizeof(ForwPair*) + sizeof(size_t));

   for(auto f = forws_->cbegin(); f != forws_->cend(); ++f)
   {
      ssize += f->first.capacity();
   }

   vsize += friends_->size() * (sizeof(FriendPair) + 2 * sizeof(FriendPair*));
   vsize += friends_->bucket_count() * (sizeof(FriendPair*) + sizeof(size_t));

   for(auto f = friends_->cbegin(); f != friends_->cend(); ++f)
   {
      ssize += f->first.capacity();
   }

   vsize += funcs_->size() * (sizeof(FuncPair) + 2 * sizeof(FuncPair*));
   vsize += funcs_->bucket_count() * (sizeof(FuncPair*) + sizeof(size_t));

   for(auto f = funcs_->cbegin(); f != funcs_->cend(); ++f)
   {
      ssize += f->first.capacity();
   }

   vsize += spaces_->size() * (sizeof(SpacePair) + 2 * sizeof(SpacePair*));
   vsize += spaces_->bucket_count() * (sizeof(SpacePair*) + sizeof(size_t));

   for(auto s = spaces_->cbegin(); s != spaces_->cend(); ++s)
   {
      ssize += s->first.capacity();
   }

   vsize += terms_->size() * (sizeof(TermPair) + 2 * sizeof(TermPair*));
   vsize += terms_->bucket_count() * (sizeof(TermPair*) + sizeof(size_t));

   for(auto t = terms_->cbegin(); t != terms_->cend(); ++t)
   {
      ssize += t->first.capacity();
   }

   vsize += types_->size() * (sizeof(TypePair) + 2 * sizeof(TypePair*));
   vsize += types_->bucket_count() * (sizeof(TypePair*) + sizeof(size_t));

   for(auto t = types_->cbegin(); t != types_->cend(); ++t)
   {
      ssize += t->first.capacity();
   }

   CxxStats::Strings(CxxStats::CXX_SYMBOLS, ssize);
   CxxStats::Vectors(CxxStats::CXX_SYMBOLS, vsize);
}

//------------------------------------------------------------------------------

void CxxSymbols::Shutdown(RestartLevel level)
{
   Debug::ft("CxxSymbols.Shutdown");

   //  Symbol tables are preserved during restarts.
   //
   if(terms_ != nullptr) return;

   classes_.reset();
   data_.reset();
   enums_.reset();
   etors_.reset();
   forws_.reset();
   friends_.reset();
   funcs_.reset();
   macros_.reset();
   spaces_.reset();
   terms_.reset();
   types_.reset();
}

//------------------------------------------------------------------------------

void CxxSymbols::Startup(RestartLevel level)
{
   Debug::ft("CxxSymbols.Startup");

   //  Create the symbol tables if they don't exist.
   //
   if(terms_ != nullptr) return;

   classes_.reset(new ClassTable);
   data_.reset(new DataTable);
   enums_.reset(new EnumTable);
   etors_.reset(new EtorTable);
   forws_.reset(new ForwTable);
   friends_.reset(new FriendTable);
   funcs_.reset(new FuncTable);
   macros_.reset(new MacroTable);
   spaces_.reset(new SpaceTable);
   terms_.reset(new TermTable);
   types_.reset(new TypeTable);
}
}
