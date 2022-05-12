//==============================================================================
//
//  CxxSymbols.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
#ifndef CXXSYMBOLS_H_INCLUDED
#define CXXSYMBOLS_H_INCLUDED

#include "Base.h"
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "Cxx.h"
#include "CxxFwd.h"
#include "CxxScoped.h"
#include "CxxString.h"
#include "LibraryTypes.h"
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Types for accessing symbol information.
//
typedef std::vector< CxxScoped* > SymbolVector;
typedef std::vector< SymbolView > ViewVector;

//  Specifies the type of item that could resolve a symbol.
//
extern const NodeBase::Flags CLASS_MASK;   // class, struct, union
extern const NodeBase::Flags DATA_MASK;    // data
extern const NodeBase::Flags ETOR_MASK;    // enumerator
extern const NodeBase::Flags ENUM_MASK;    // enum
extern const NodeBase::Flags FORW_MASK;    // forward declaration
extern const NodeBase::Flags FRIEND_MASK;  // friend declaration
extern const NodeBase::Flags FUNC_MASK;    // function
extern const NodeBase::Flags MACRO_MASK;   // #define
extern const NodeBase::Flags SPACE_MASK;   // namespace
extern const NodeBase::Flags TERM_MASK;    // terminal (built-in type)
extern const NodeBase::Flags TYPE_MASK;    // typedef

//  Combinations of the above, used when searching in various situations.
//  o CODE_REFS includes all items except namespaces, locals, and terminals.
//  o ITEM_REFS includes all items except locals and terminals.
//  o CLASS_FORWS are forward declarations of a class for the >rename command.
//  o FRIEND_CLASSES are used when a friend is a class.
//  o FRIEND_FUNCS are used when a friend is a function.
//  o FUNC_FORWS are forward declarations of a function for the >rename command.
//  o RENAME_REFS are items that can be specified in the >rename command.
//  o SCOPE_REFS are items that can precede a scope resolution operator.
//  o TARG_REFS finds a template argument.
//  o TYPE_REFS finds the result of an operator (bool, size_t, or name_info).
//  o TYPESPEC_REFS are referents of a TypeSpec.
//  o USING_REFS are referents of a using statement.
//  o VALUE_REFS are storage references or constants.
//
extern const NodeBase::Flags CLASS_FORWS;
extern const NodeBase::Flags CODE_REFS;
extern const NodeBase::Flags ITEM_REFS;
extern const NodeBase::Flags FRIEND_CLASSES;
extern const NodeBase::Flags FRIEND_FUNCS;
extern const NodeBase::Flags FUNC_FORWS;
extern const NodeBase::Flags RENAME_REFS;
extern const NodeBase::Flags SCOPE_REFS;
extern const NodeBase::Flags TARG_REFS;
extern const NodeBase::Flags TYPE_REFS;
extern const NodeBase::Flags TYPESPEC_REFS;
extern const NodeBase::Flags USING_REFS;
extern const NodeBase::Flags VALUE_REFS;

//------------------------------------------------------------------------------
//
//  Symbol database.
//
class CxxSymbols: public NodeBase::Base
{
   friend class NodeBase::Singleton< CxxSymbols >;
public:
   //  Deleted to prohibit copying.
   //
   CxxSymbols(const CxxSymbols& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   CxxSymbols& operator=(const CxxSymbols& that) = delete;

   //  Returns any terminal(s) that match NAME.
   //
   void FindTerminal(const std::string& name, SymbolVector& list) const;

   //  Returns the item referred to by NAME, which was used in FILE and SCOPE.
   //  If AREA is provided, only items in that area are considered.  Returns
   //  nullptr if no item was found.  When an item is returned, VIEW is updated
   //  with details on how it was found.  MASK specifies the types of items to
   //  search for (see the constants defined above).
   //
   CxxScoped* FindSymbol(CodeFile* file, const CxxScope* scope,
      const std::string& name, const NodeBase::Flags& mask, SymbolView& view,
      const CxxArea* area = nullptr) const;

   //  The same as FindSymbol, but returns all matching symbols in LIST,
   //  along with their VIEWS.
   //
   void FindSymbols(CodeFile* file, const CxxScope* scope,
      const std::string& name, const NodeBase::Flags& mask, SymbolVector& list,
      ViewVector& views, const CxxArea* area = nullptr) const;

   //  The same as FindSymbol, but returns all matching symbols in LIST.
   //  Used to find items that can match a name entered in a CLI command.
   //
   void FindItems(const std::string& name,
      const NodeBase::Flags& mask, SymbolVector& list) const;

   //  Returns the scope (namespace, class, or function) referred to by
   //  NAME, which was used in SCOPE.
   //
   CxxScope* FindScope(const CxxScope* scope, std::string& name) const;

   //  Returns the macro identified by NAME, whether it has been defined
   //  or has only appeared as a symbol.
   //
   Macro* FindMacro(const std::string& name) const;

   //  Returns true if SCOPE only contains one function with NAME.
   //
   bool IsUniqueName(const CxxScope* scope, const std::string& name) const;

   //  Adds the specified item to the symbol database.
   //
   void InsertClass(Class* cls);
   void InsertData(Data* data);
   void InsertEtor(Enumerator* etor);
   void InsertEnum(Enum* item);
   void InsertForw(Forward* forw);
   void InsertFriend(Friend* frnd);
   void InsertFunc(Function* func);
   void InsertMacro(Macro* macro);
   void InsertSpace(Namespace* space);
   void InsertTerm(Terminal* term);
   void InsertType(Typedef* type);

   //  Removes the specified item from the symbol database.
   //
   void EraseClass(const Class* cls);
   void EraseData(const Data* data);
   void EraseEtor(const Enumerator* etor);
   void EraseEnum(const Enum* item);
   void EraseForw(const Forward* forw);
   void EraseFriend(const Friend* frnd);
   void EraseFunc(const Function* func);
   void EraseMacro(const Macro* macro);
   void EraseSpace(const Namespace* space);
   void EraseTerm(const Terminal* term);
   void EraseType(const Typedef* type);

   //  Outputs the global cross-reference to STREAM.  The characters in
   //  OPTS control what information will be included.
   //
   void DisplayXref(std::ostream& stream, const std::string& opts) const;

   //  Overridden for restarts.
   //
   void Shutdown(NodeBase::RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Startup(NodeBase::RestartLevel level) override;
private:
   //  Adds any macros identified by NAME to LIST, but only those that
   //  have been defined.
   //
   void ListMacros(const std::string& name, SymbolVector& list) const;

   //  Types for symbol tables.
   //
   typedef std::unordered_multimap< std::string, Class* > ClassTable;
   typedef std::unordered_multimap< std::string, Data* > DataTable;
   typedef std::unordered_multimap< std::string, Enum* > EnumTable;
   typedef std::unordered_multimap< std::string, Enumerator* > EtorTable;
   typedef std::unordered_multimap< std::string, Forward* > ForwTable;
   typedef std::unordered_multimap< std::string, Friend* > FriendTable;
   typedef std::unordered_multimap< std::string, Function* > FuncTable;
   typedef std::unordered_multimap< std::string, Macro* > MacroTable;
   typedef std::unordered_multimap< std::string, Namespace* > SpaceTable;
   typedef std::unordered_multimap< std::string, Terminal* > TermTable;
   typedef std::unordered_multimap< std::string, Typedef* > TypeTable;

   //  Types for unique_ptrs that own symbol tables.
   //
   typedef std::unique_ptr< ClassTable > ClassTablePtr;
   typedef std::unique_ptr< DataTable > DataTablePtr;
   typedef std::unique_ptr< EnumTable > EnumTablePtr;
   typedef std::unique_ptr< EtorTable > EtorTablePtr;
   typedef std::unique_ptr< ForwTable > ForwTablePtr;
   typedef std::unique_ptr< FriendTable > FriendTablePtr;
   typedef std::unique_ptr< FuncTable > FuncTablePtr;
   typedef std::unique_ptr< MacroTable > MacroTablePtr;
   typedef std::unique_ptr< SpaceTable > SpaceTablePtr;
   typedef std::unique_ptr< TermTable > TermTablePtr;
   typedef std::unique_ptr< TypeTable > TypeTablePtr;

   //  Private because this is a singleton.
   //
   CxxSymbols();

   //  Private because this is a singleton.
   //
   ~CxxSymbols();

   //  Symbol tables.
   //
   ClassTablePtr classes_;
   DataTablePtr data_;
   EnumTablePtr enums_;
   EtorTablePtr etors_;
   ForwTablePtr forws_;
   FriendTablePtr friends_;
   FuncTablePtr funcs_;
   MacroTablePtr macros_;
   TermTablePtr terms_;
   SpaceTablePtr spaces_;
   TypeTablePtr types_;
};

//------------------------------------------------------------------------------
//
//  Returns the index of the item in LIST that is nearest the context scope.
//  Returns SIZE_MAX if none of the items in LIST is in a related scope.
//
size_t FindNearestItem(const SymbolVector& list);

//------------------------------------------------------------------------------
//
//  Removes ITEM from TABLE.
//
template< typename T > void EraseSymbol(const CxxScoped* item,
   std::unordered_multimap< std::string, T >& table)
{
   auto str = Normalize(item->Name());
   auto range = table.equal_range(str);

   for(auto i = range.first; i != range.second; ++i)
   {
      if(i->second == item)
      {
         table.erase(i);
         return;
      }
   }
}

//------------------------------------------------------------------------------
//
//  Looks for NAME in TABLE.  Returns a list of matching symbols in LIST.
//
template< typename T > void ListSymbols(const std::string& name,
   const std::unordered_multimap< std::string, T >& table, SymbolVector& list)
{
   //  Assemble a list of matching symbols.
   //
   auto range = table.equal_range(name);

   for(auto i = range.first; i != range.second; ++i)
   {
      list.push_back(i->second);
   }
}
}
#endif
