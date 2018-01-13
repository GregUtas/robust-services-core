//==============================================================================
//
//  CxxSymbols.h
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
#ifndef CXXSYMBOLS_H_INCLUDED
#define CXXSYMBOLS_H_INCLUDED

#include "Temporary.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "CxxFwd.h"
#include "NbTypes.h"
#include "SysTypes.h"

namespace CodeTools
{
   struct SymbolView;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Types for accessing symbol information.
//
typedef std::vector< CxxScoped* > SymbolVector;
typedef std::vector< SymbolView > ViewVector;

//  Specifies the type of item that could resolve a symbol.
//
extern const Flags CLASS_MASK;   // class, struct, union
extern const Flags DATA_MASK;    // data
extern const Flags ETOR_MASK;    // enumerator
extern const Flags ENUM_MASK;    // enum
extern const Flags FORW_MASK;    // forward declaration
extern const Flags FRIEND_MASK;  // friend declaration
extern const Flags FUNC_MASK;    // function
extern const Flags MACRO_MASK;   // #define
extern const Flags SPACE_MASK;   // namespace
extern const Flags TERM_MASK;    // terminal (built-in type)
extern const Flags TYPE_MASK;    // typedef

//  Combinations of the above, used when searching in various situations.
//  o CODE_REFS includes all items except locals and terminals.
//  o FRIEND_CLASSES are used when a friend is a class.
//  o FRIEND_FUNCS are used when a friend is a function.
//  o SCOPE_REFS are items that can precede a scope resolution operator.
//  o TYPE_REFS finds the result of an operator (bool, size_t, or name_info).
//  o TYPESPEC_REFS are referents of a TypeSpec.
//  o USING_REFS are referents of a using statement.
//  o VALUE_REFS are storage references or constants.
//
extern const Flags CODE_REFS;
extern const Flags FRIEND_CLASSES;
extern const Flags FRIEND_FUNCS;
extern const Flags SCOPE_REFS;
extern const Flags TYPE_REFS;
extern const Flags TYPESPEC_REFS;
extern const Flags USING_REFS;
extern const Flags VALUE_REFS;

//------------------------------------------------------------------------------
//
//  Symbol database.
//
class CxxSymbols: public Temporary
{
   friend class Singleton< CxxSymbols >;
public:
   //  Returns the item referred to by NAME, which was used in FILE and SCOPE.
   //  If AREA is provided, only items in that area are considered.  Returns
   //  nullptr if no item was found.  When an item is returned, VIEW is updated
   //  with details on how it was found.  MASK specifies the types of items to
   //  search for (see the constants defined above).
   //
   CxxScoped* FindSymbol(const CodeFile* file, const CxxScope* scope,
      const std::string& name, const Flags& mask, SymbolView* view,
      const CxxArea* area = nullptr) const;

   //  Returns NAME if it is a terminal or a local variable in a function.
   //
   CxxScoped* FindLocal(const std::string& name, SymbolView* view) const;

   //  The same as FindSymbol, but returns all matching symbols in LIST,
   //  along with their VIEWS.
   //
   void FindSymbols(const CodeFile* file, const CxxScope* scope,
      const std::string& name, const Flags& mask, SymbolVector& list,
      ViewVector& views, const CxxArea* area = nullptr) const;

   //  Returns the scope (namespace, class, or function) referred to by
   //  NAME, which was used in SCOPE.
   //
   CxxScope* FindScope(const CxxScope* scope, std::string& name) const;

   //  Returns the macro identified by NAME, whether it has been defined
   //  or has only appeared as a symbol.
   //
   Macro* FindMacro(const std::string& name) const;

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
   void InsertLocal(CxxScoped* local);

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
   void EraseLocal(const CxxScoped* name);

   //  Releases all entries in the local symbol table.
   //
   void EraseLocals();

   //  Shrinks containers.
   //
   void Shrink() const;

   //  Overridden for restarts.
   //
   virtual void Shutdown(RestartLevel level) override;

   //  Overridden for restarts.
   //
   virtual void Startup(RestartLevel level) override;
private:
   //  Adds any macros identified by NAME to LIST, but only those that
   //  have been defined.
   //
   void ListMacros(const std::string& name, SymbolVector& list) const;

   //  Types for symbol tables.
   //
   typedef std::unordered_multimap< std::string, CxxScoped* > LocalTable;
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
   typedef std::unique_ptr< LocalTable > LocalTablePtr;
   typedef std::unique_ptr< MacroTable > MacroTablePtr;
   typedef std::unique_ptr< SpaceTable > SpaceTablePtr;
   typedef std::unique_ptr< TermTable > TermTablePtr;
   typedef std::unique_ptr< TypeTable > TypeTablePtr;

   //  Private because this singleton is not subclassed.
   //
   CxxSymbols();

   //  Private because this singleton is not subclassed.
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
   LocalTablePtr locals_;
   MacroTablePtr macros_;
   TermTablePtr terms_;
   SpaceTablePtr spaces_;
   TypeTablePtr types_;
};
}
#endif
