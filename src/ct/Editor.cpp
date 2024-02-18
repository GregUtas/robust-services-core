//==============================================================================
//
//  Editor.cpp
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
#include "Editor.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iosfwd>
#include <iterator>
#include <list>
#include <sstream>
#include "CliThread.h"
#include "CodeCoverage.h"
#include "CodeFile.h"
#include "CxxArea.h"
#include "CxxDirective.h"
#include "CxxLocation.h"
#include "CxxNamed.h"
#include "CxxRoot.h"
#include "CxxScope.h"
#include "CxxScoped.h"
#include "CxxString.h"
#include "CxxStrLiteral.h"
#include "CxxSymbols.h"
#include "CxxToken.h"
#include "CxxVector.h"
#include "Debug.h"
#include "Duration.h"
#include "FileSystem.h"
#include "Formatters.h"
#include "Library.h"
#include "LibraryItem.h"
#include "NbCliParms.h"
#include "Parser.h"
#include "Singleton.h"
#include "ThisThread.h"

using std::ostream;
using std::string;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  The editors that have modified their original code.  This allows multiple
//  files to be changed (e.g. when a fix requires changes in both a function's
//  declaration and definition).  After all changes needed for a fix have been
//  made, all modified files can be committed.
//
static std::set<Editor*> Editors_;

//  The number of files committed so far.
//
static size_t Commits_ = 0;

//  The CLI thread on which Fix was invoked.  This allows an Editor function
//  to display results without needing CliThread& as one of its parameters.
//
static CliThread* Cli_ = nullptr;

//  Writes TEXT to Cli_, adding an endline if one doesn't exist.
//
static void Inform(string text);

//------------------------------------------------------------------------------
//
//  Return codes from editor functions.
//
constexpr word EditAbort = -2;     // stream error or fix not implemented
constexpr word EditFailed = -1;    // could not fix; write info to CLI
constexpr word EditSucceeded = 0;  // warning fixed; write info to CLI
constexpr word EditContinue = 0;   // continue with edit
constexpr word EditCompleted = 1;  // warning closed; don't write info to CLI

//  The last result.  It, and the two functions that manage it, are similar
//  to the SetLastError/GetLastError interface.  Based on the return code,
//  Expl_ should contain
//  o EditFailed or EditAbort: an explanation of why the edit failed
//  o EditSucceeded: an edited line of code (or an empty string after deletion)
//  o EditContinue: an empty string, because there is nothing to report
//  o EditCompleted: an empty string, because all results have been reported
//
static string Expl_;

//  Sets Expl_ to EXPL, adding a CRLF if EXPL doesn't have one.
//
static void SetExpl(const string& expl);

//  Returns Expl_ after clearing it.
//
static string GetExpl();

//------------------------------------------------------------------------------
//
//  Strings for user interaction.
//
fixed_string FixPrompt = "Fix?";
fixed_string YNSQHelp = "Enter y(yes) n(no) s(skip file) q(quit): ";
fixed_string YNSQChars = "ynsq";
fixed_string FixSkipped = "This fix will be skipped.";
fixed_string ItemDeleted = "The item associated with this warning was deleted.";
fixed_string NotImplemented = "Fixing this warning is not yet supported.";
fixed_string UnspecifiedFailure = "Internal error. Edit failed.";

fixed_string AccessPrompt = "Enter 0=skip 1=public 2=protected 3=private: ";
fixed_string ArgPrompt = "Choose the argument's name. ";
fixed_string DefnPrompt = "0=skip 1=default 2=delete: ";
fixed_string FilePrompt = "Enter the filename in which to define ";
fixed_string ShellPrompt = "Do you want to create a shell for the definition?";
fixed_string NewNamePrompt = "Enter a new name, or an illegal name to skip: ";
fixed_string SuffixPrompt = "Enter a suffix for the name: ";
fixed_string VirtualPrompt = "Enter 0=skip 1=virtual 2=non-virtual: ";
fixed_string CommitFailedPrompt = "Do you want to retry?";

fixed_string ClassInstantiated = "Objects of this class are created, "
                                 "so its constructor must remain public.";

//==============================================================================
//
//  Where to add a blank line when declaring a C++ item.
//
enum BlankLocation
{
   BlankNone,
   BlankAbove,
   BlankBelow
};

//------------------------------------------------------------------------------
//
//  Attributes when declaring a C++ item.
//
struct ItemDeclAttrs
{
   //  Initializes the attributes for an item of type T, with access control A.
   //
   ItemDeclAttrs(Cxx::ItemType t, Cxx::Access a, FunctionRole r = FuncOther);

   //  Initializes the attributes for ITEM.
   //
   explicit ItemDeclAttrs(const CxxToken* item);

   //  Returns the order, within a class, where the item should be declared.
   //
   size_t CalcDeclOrder() const;

   //  The following are provided as inputs.
   //
   const Cxx::ItemType type_;  // type of item being declared
   Cxx::Access access_;        // desired access control
   FunctionRole role_;         // if a function, the type being added
   bool over_;                 // set if a function is an override

   //  The following are calculated internally.
   //
   bool isstruct_;         // set if item belongs to a struct
   bool oper_;             // set for an operator
   bool virt_;             // set to tag a function as virtual
   bool deleted_;          // set to define a function as deleted
   bool shell_;            // set to create a shell for defining a function
   bool stat_;             // set for a static function or data
   const CxxToken* prev_;  // item immediately before insertion point
   const CxxToken* next_;  // item immediately after insertion point
   bool thisctrl_;         // set to insert access control before
   Cxx::Access nextctrl_;  // set to insert access control after
   size_t pos_;            // position for insertion
   size_t indent_;         // number of spaces for indentation
   BlankLocation blank_;   // where to insert a blank line
   bool comment_;          // set to include a comment
};

//------------------------------------------------------------------------------

ItemDeclAttrs::ItemDeclAttrs(Cxx::ItemType t, Cxx::Access a, FunctionRole r) :
   type_(t),
   access_(a),
   role_(r),
   over_(false),
   isstruct_(false),
   oper_(false),
   virt_(false),
   deleted_(false),
   shell_(false),
   stat_(false),
   prev_(nullptr),
   next_(nullptr),
   thisctrl_(false),
   nextctrl_(Cxx::Access_N),
   pos_(string::npos),
   indent_(0),
   blank_(BlankNone),
   comment_(false)
{
   Debug::ft("ItemDeclAttrs.ctor(type)");
}

//------------------------------------------------------------------------------

ItemDeclAttrs::ItemDeclAttrs(const CxxToken* item) :
   type_(item->Type()),
   access_(item->GetAccess()),
   role_(FuncOther),
   over_(false),
   isstruct_(false),
   oper_(false),
   virt_(false),
   deleted_(false),
   shell_(false),
   stat_(false),
   prev_(nullptr),
   next_(nullptr),
   thisctrl_(false),
   nextctrl_(Cxx::Access_N),
   pos_(string::npos),
   indent_(0),
   blank_(BlankNone),
   comment_(false)
{
   Debug::ft("ItemDeclAttrs.ctor(item)");

   switch(type_)
   {
   case Cxx::Function:
   {
      auto func = static_cast<const Function*>(item);
      role_ = func->FuncRole();
      over_ = func->IsOverride();
      oper_ = (func->FuncType() == FuncOperator);
      virt_ = !over_ && func->IsVirtual();
      [[fallthrough]];
   }
   case Cxx::Data:
      stat_ = item->IsStatic();
      break;
   }

   auto cls = item->GetClass();
   if(cls != nullptr) isstruct_ = (cls->GetClassTag() != Cxx::ClassType);

   const auto& lexer = item->GetFile()->GetLexer();
   size_t begin, end;

   if(item->GetSpan2(begin, end))
   {
      indent_ = lexer.LineFindFirst(begin) - lexer.CurrBegin(begin);
   }
}

//------------------------------------------------------------------------------

fn_name ItemDeclAttrs_CalcDeclOrder = "ItemDeclAttrs.CalcDeclOrder";

size_t ItemDeclAttrs::CalcDeclOrder() const
{
   Debug::ft(ItemDeclAttrs_CalcDeclOrder);

   //  Items have to be declared in some order, so this tries to organize
   //  them in a consistent way. The first thing that determines order is
   //  the item's access control (except that a friend declaration always
   //  appears first).
   //
   size_t order = 0;

   switch(access_)
   {
   case Cxx::Public:
      order = 0;
      break;
   case Cxx::Protected:
      order = 20;
      break;
   default:
      order = 40;
   }

   switch(type_)
   {
   case Cxx::Friend:
      return 0;
   case Cxx::Forward:
      return order + 3;
   case Cxx::Enum:
      return order + 4;
   case Cxx::Typedef:
      return order + 5;
   case Cxx::Class:
      return order + 6;

   case Cxx::Function:
   {
      switch(role_)
      {
      case PureCtor:
         return order + 7;
      case PureDtor:
         return order + 8;
      case CopyCtor:
         return order + 9;
      case MoveCtor:
         return order + 10;
      case CopyOper:
         return order + 11;
      case MoveOper:
         return order + 12;
      default:
         if(oper_) return order + 13;
         if(virt_) return order + 14;
         if(over_) return order + 17;
         if(stat_) return order + 16;
         return order + 15;
      }
   }

   case Cxx::Data:
      if(isstruct_)
      {
         //  If a struct defines its data first, this will prevent functions
         //  from being added before its data.
         //
         if(stat_) return order + 2;
         return order + 1;
      }
      else
      {
         if(stat_) return order + 19;
         return order + 18;
      }
   }

   Debug::SwLog(ItemDeclAttrs_CalcDeclOrder, "unexpected item type", type_);
   return order;
}

//==============================================================================
//
//  How an item's definition is separated from other code.
//
enum ItemOffset
{
   OffsetNone,   // not offset
   OffsetBlank,  // blank line
   OffsetRule    // blank line and rule
};

struct ItemOffsets
{
   ItemOffsets() : above_(OffsetNone), below_(OffsetNone) { }

   ItemOffset above_;  // offset above the item
   ItemOffset below_;  // offset below the item
};

//------------------------------------------------------------------------------
//
//  Attributes when inserting an item definition.
//
struct ItemDefnAttrs
{
   ItemDefnAttrs();
   explicit ItemDefnAttrs(FunctionRole role);
   explicit ItemDefnAttrs(const Function* func);

   FunctionRole role_;    // type of function being inserted
   size_t pos_;           // insertion position
   ItemOffsets offsets_;  // how to offset item
};

ItemDefnAttrs::ItemDefnAttrs() :
   role_(FuncRole_N),
   pos_(string::npos)
{
   Debug::ft("ItemDefnAttrs.ctor");
}

ItemDefnAttrs::ItemDefnAttrs(FunctionRole role) :
   role_(role),
   pos_(string::npos)
{
   Debug::ft("ItemDefnAttrs.ctor(role)");
}

ItemDefnAttrs::ItemDefnAttrs(const Function* func) :
   role_(func->FuncRole()),
   pos_(string::npos)
{
   Debug::ft("ItemDefnAttrs.ctor(func)");
}

//------------------------------------------------------------------------------
//
//  Asks the user to choose declName or defnName as an argument's name.
//
static string ChooseArgumentName(const string& declName, const string& defnName)
{
   Debug::ft("CodeTools.ChooseArgumentName");

   std::ostringstream stream;
   stream << spaces(2) << ArgPrompt << CRLF;
   stream << spaces(4) << "1=" << QUOTE << declName << QUOTE << SPACE;
   stream << spaces(4) << "2=" << QUOTE << defnName << QUOTE << ": ";
   auto choice = Cli_->IntPrompt(stream.str(), 1, 2);
   return (choice == 1 ? declName : defnName);
}

//------------------------------------------------------------------------------
//
//  Asks the user to determine the attributes for a destructor that is
//  currently non-virtual.
//
static word ChooseDtorAttributes(ItemDeclAttrs& attrs)
{
   Debug::ft("CodeTools.ChooseDtorAttributes");

   std::ostringstream stream;
   stream << spaces(2) << AccessPrompt;
   auto response = Cli_->IntPrompt(stream.str(), 0, 3);

   auto access = Cxx::Access_N;
   auto virt = false;

   switch(response)
   {
   case 1:
      access = Cxx::Public;
      break;
   case 2:
      access = Cxx::Protected;
      break;
   case 3:
      access = Cxx::Private;
      break;
   default:
      return EditFailed;
   }

   stream.str(EMPTY_STR);
   stream << spaces(2) << VirtualPrompt;
   response = Cli_->IntPrompt(stream.str(), 0, 2);

   switch(response)
   {
   case 1:
      virt = true;
      break;
   case 2:
      virt = false;
      break;
   default:
      return EditFailed;
   }

   if((attrs.access_ == access) && (attrs.virt_ == virt)) return EditCompleted;
   attrs.access_ = access;
   attrs.virt_ = virt;
   return EditSucceeded;
}

//------------------------------------------------------------------------------
//
//  Creates a comment for the special member function that will be added
//  to CLS according to ATTRS.
//
static string CreateSpecialFunctionComment
   (const Class* cls, const ItemDeclAttrs& attrs)
{
   Debug::ft("CodeTools.CreateSpecialFunctionComment");

   string comment;

   if(attrs.deleted_)
   {
      //  Currently occurs only for a copy constructor/operator.
      //
      switch(attrs.role_)
      {
      case CopyCtor:
      case MoveCtor:
         comment = "Deleted to prohibit copying";
         break;
      case CopyOper:
      case MoveOper:
         comment = "Deleted to prohibit copy assignment";
         break;
      case PureCtor:
      case PureDtor:
         comment = "Deleted because this class only has static members";
         break;
      default:
         comment = "[Add comment.]";
      }
   }
   else if(attrs.virt_)
   {
      //  For a base class destructor.
      //
      if(cls->IsSingletonBase())
         comment = "Protected because subclasses should be singletons";
      else
         comment = "Virtual to allow subclassing";
   }
   else if(attrs.access_ == Cxx::Protected)
   {
      //  For a base class (other than the destructor).
      //
      comment = "Protected because this class is virtual";
   }
   else if(attrs.access_ == Cxx::Private)
   {
      //  For a singleton's constructor or destructor.
      //
      comment = "Private because this is a singleton";
   }
   else if(attrs.role_ == PureDtor)
   {
      //  For a leaf class destructor.
      //
      comment = "Not subclassed";
   }
   else
   {
      //  None of the above applies.
      //
      std::ostringstream stream;
      stream << attrs.role_;
      comment = stream.str();
      comment.front() = toupper(comment.front());
   }

   comment.push_back('.');
   return comment;
}

//------------------------------------------------------------------------------
//
//  Returns the code for a Debug::ft invocation with an inline string
//  literal (FNAME).
//
static string DebugFtCode(const string& fname)
{
   Debug::ft("CodeTools.DebugFtCode");

   auto call = string(IndentSize(), SPACE) + "Debug::ft(";
   call.append(fname);
   call.append(");");
   return call;
}

//------------------------------------------------------------------------------
//
//  Returns an unquoted string (FLIT) and fn_name identifier (FVAR) that are
//  suitable for invoking Debug::ft.
//
static void DebugFtNames(const Function* func, string& flit, string& fvar)
{
   Debug::ft("CodeTools.DebugFtNames");

   //  Get the function's name and the area in which it appears.
   //
   const auto& sname = func->GetArea()->Name();
   auto fname = func->DebugName();

   flit = sname;
   if(!sname.empty()) flit.push_back('.');
   flit.append(fname);

   fvar = sname;
   if(!fvar.empty()) fvar.push_back('_');

   if(func->FuncType() == FuncOperator)
   {
      //  Something like "class_operator=" won't pass as an identifier, so use
      //  "class_operatorN", where N is the integer value of the Operator enum.
      //
      auto oper = CxxOp::NameToOperator(fname);
      fname.erase(strlen(OPERATOR_STR));
      fname.append(std::to_string(oper));
   }

   fvar.append(fname);
}

//------------------------------------------------------------------------------
//
//  Sets FNAME to FLIT, the argument for Debug::ft.  If it is already in use,
//  prompts the user for a suffix to make it unique.  Returns false if FNAME
//  is not unique and the user did not provide a satisfactory suffix, else
//  returns true after enclosing FNAME in quotes.
//
static bool EnsureUniqueDebugFtName
   (const Function* func, const string& flit, string& fname)
{
   Debug::ft("CodeTools.EnsureUniqueDebugFtName");

   auto coverdb = Singleton<CodeCoverage>::Instance();
   auto hash = func->CalcHash();
   fname = flit;

   while(!coverdb->Insert(fname, hash))
   {
      std::ostringstream stream;
      stream << spaces(2) << fname << " is already in use. " << SuffixPrompt;
      auto suffix = Cli_->StrPrompt(stream.str());
      if(suffix.empty()) return false;
      fname = flit + '(' + suffix + ')';
   }

   fname.insert(0, 1, QUOTE);
   fname.push_back(QUOTE);
   return true;
}

//------------------------------------------------------------------------------
//
//  Returns the file where a class's new function should be defined.  If all of
//  the its functions are implemented in the same file, returns that file, else
//  asks the user to specify the file.
//
static CodeFile* FindFuncDefnFile(const Class* cls, const string& name)
{
   Debug::ft("CodeTools.FindFuncDefnFile");

   std::set<CodeFile*> impls;
   auto funcs = cls->Funcs();

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      auto file = (*f)->GetDefnFile();
      if((file != nullptr) && file->IsCpp()) impls.insert(file);
   }

   CodeFile* file = (impls.size() == 1 ? *impls.cbegin() : nullptr);

   while(file == nullptr)
   {
      std::ostringstream prompt;
      prompt << spaces(2) << FilePrompt << CRLF;
      prompt << spaces(4) << cls->Name() << SCOPE_STR << name;
      prompt << " ('s' to skip this item): ";
      auto fileName = Cli_->StrPrompt(prompt.str());
      if(fileName == "s") return nullptr;

      file = Singleton<Library>::Instance()->FindFile(fileName);
      if(file == nullptr)
      {
         Inform("That file is not in the code library.");
      }
   }

   return file;
}

//------------------------------------------------------------------------------
//
//  Returns the namespace identified by NSPACE.
//
static Namespace* FindNamespace(const string& nspace)
{
   Debug::ft("CodeTools.FindNamespace");

   string space(nspace);
   auto syms = Singleton<CxxSymbols>::Instance();
   auto gns = Singleton<CxxRoot>::Instance()->GlobalNamespace();
   auto scope = (nspace != EMPTY_STR ? syms->FindScope(gns, space) : gns);
   return static_cast<Namespace*>(scope);
}

//------------------------------------------------------------------------------
//
//  Returns the comment, if any, associated with the declaration of ITEM after
//  removing the indentation on each line if UNINDENT is set.
//
static string GetComment(const CxxNamed* item, bool unindent)
{
   Debug::ft("CodeTools.GetComment");

   size_t begin, end;
   if(!item->GetSpan2(begin, end)) return EMPTY_STR;

   const auto& lexer = item->GetFile()->GetLexer();
   begin = lexer.CurrBegin(begin);
   auto start = lexer.IntroStart(begin, false);

   if(start == begin) return EMPTY_STR;
   auto comment = lexer.Source().substr(start, begin - start);

   if(!unindent) return comment;

   for(begin = 0; begin < comment.size(); begin = comment.find(CRLF, begin) + 1)
   {
      auto pos = comment.find(COMMENT_STR, begin);
      if(pos == string::npos) break;
      comment.erase(begin, pos - begin);
   }

   return comment;
}

//------------------------------------------------------------------------------
//
//  Adds DATA and its separate definition, if it exists, to DATAS.
//
static void GetDatas(Data* data, DataVector& datas)
{
   Debug::ft("CodeTools.GetDatas");

   auto defn = static_cast<Data*>(data->GetMate());
   if(defn != nullptr) datas.push_back(defn);
   datas.push_back(data);
}

//------------------------------------------------------------------------------

static string GetExpl()
{
   Debug::ft("CodeTools.GetExpl");

   auto expl = Expl_;
   Expl_.clear();
   return expl;
}

//------------------------------------------------------------------------------
//
//  Writes PROMPT to the CLI to obtain an identifier.  Returns the identifier
//  if it is valid, else returns an empty string.
//
static string GetIdentifier(const string& prompt)
{
   Debug::ft("CodeTools.GetIdentifier");

   auto name = Cli_->StrPrompt(prompt);

   if(name.empty()) return EMPTY_STR;

   if(ValidFirstChars.find(name[0]) == string::npos) return EMPTY_STR;
   if(name[0] == '#') return EMPTY_STR;
   if(name[0] == '~') return EMPTY_STR;

   for(size_t i = 1; i < name.size(); ++i)
   {
      if(ValidNextChars.find(name[i]) == string::npos) return EMPTY_STR;
      if(name[i] == '#') return EMPTY_STR;
   }

   return name;
}

//------------------------------------------------------------------------------
//
//  Checks that the name of member DECL is not in use when planning to convert
//  DECL to a static namespace item in the FILE that implements the class that
//  currently declares it.  If the name is in use, prompts for and returns an
//  alternate name if provided, else returns an empty string.
//
static string GetItemName(const CxxScope* decl, CodeFile* file)
{
   Debug::ft("CodeTools.GetItemName");

   auto syms = Singleton<CxxSymbols>::Instance();
   auto space = decl->GetSpace();
   auto name = decl->Name();

   while(true)
   {
      SymbolView view;
      auto item = syms->FindSymbol(file, space, name, CODE_REFS, view);
      if(item == nullptr) return name;

      std::ostringstream stream;
      stream << spaces(2) << name << " is already in use." << CRLF;
      name = GetIdentifier(NewNamePrompt);
      if(name.empty()) return name;
   }

   return name;
}

//------------------------------------------------------------------------------
//
//  Adds FUNC and its overrides to FUNCS.
//
static void GetOverrides(Function* func, FunctionVector& funcs)
{
   Debug::ft("CodeTools.GetOverrides");

   const auto& overs = func->GetOverrides();

   for(auto f = overs.cbegin(); f != overs.cend(); ++f)
   {
      GetOverrides(*f, funcs);
   }

   auto defn = static_cast<Function*>(func->GetMate());
   if((defn != nullptr) && (defn != func)) funcs.push_back(defn);

   funcs.push_back(func);
}

//------------------------------------------------------------------------------

static void Inform(string text)
{
   Debug::ft("CodeTools.Inform");

   if(!text.empty() && (text.back() != CRLF)) text.push_back(CRLF);
   Cli_->Inform(text);
}

//------------------------------------------------------------------------------
//
//  Returns true if ITEM, whose CODE is provided, is a non-trivial inline.
//  To be an inline, it must be a function without a separate definition, but
//  it must have an implementation (as opposed to being deleted or defaulted).
//  To be non-trivial, its code must span at least three lines.
//
static bool IsNonTrivialInline(const CxxToken* item, const string& code)
{
   Debug::ft("CodeTools.IsNonTrivialInline");

   if(item->Type() != Cxx::Function) return false;
   auto func = static_cast<const Function*>(item);

   if(func->GetDefn() != func) return false;
   auto impl = func->GetImpl();
   if(impl == nullptr) return false;

   for(size_t i = 0, n = 0; i < code.size(); ++i)
   {
      if(code[i] == CRLF)
      {
         if(++n > 2) return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------
//
//  Returns true if an item between BEGIN and END references ITEM.
//
static bool ItemIsUsedBetween(const CxxToken* item, size_t begin, size_t end)
{
   Debug::ft("CodeTools.ItemIsUsedBetween");

   auto xref = item->Xref();
   if(xref == nullptr) return false;

   for(auto r = xref->cbegin(); r != xref->cend(); ++r)
   {
      auto pos = (*r)->GetPos();
      if((pos >= begin) && (pos <= end)) return true;
   }

   return false;
}

//------------------------------------------------------------------------------
//
//  Sets Expl_ to "TEXT not found."  If QUOTES is set, TEXT is enclosed in
//  quotes.  Returns 0.
//
static word NotFound(c_string text, bool quotes = false)
{
   Debug::ft("CodeTools.NotFound");

   string expl;
   if(quotes) expl = QUOTE;
   expl += text;
   if(quotes) expl.push_back(QUOTE);
   expl += " not found.";
   SetExpl(expl);
   return EditFailed;
}

//------------------------------------------------------------------------------
//
//  Updates CODE by going through ITEMS and qualifying each name defined
//  in CLS (or one of its base classes).
//
static void QualifyClassItems
   (const Class* cls, const CxxNamedSet& items, string& code)
{
   Debug::ft("CodeTools.QualifyClassItems");

   const auto& className = cls->Name();
   Lexer lexer;
   lexer.Initialize(code);
   string id;

   for(auto i = items.cbegin(); i != items.cend(); ++i)
   {
      //  Don't qualify a class name.
      //
      if((*i)->Type() == Cxx::Class) continue;

      auto icls = (*i)->GetClass();

      if((cls == icls) || cls->DerivesFrom(icls))
      {
         //  Don't qualify a class name in the hierarchy, which can appear
         //  in types and constructor invocations.
         //
         const auto& name = (*i)->Name();
         if((name == className) || (name == icls->Name())) continue;

         auto pos = lexer.Find(0, STATIC_STR) + strlen(STATIC_STR) + 1;

         while(lexer.FindIdentifier(pos, id, false) && (pos <= code.size()))
         {
            if(id == name)
            {
               auto insert = true;
               auto prev1 = lexer.RfindNonBlank(pos - 1);
               auto prev2 = lexer.RfindNonBlank(prev1 - 1);
               auto char1 = code[prev1];
               auto char2 = code[prev2];

               //  Don't qualify a name that is preceded by a selection or
               //  scope resolution operator.
               //
               if(char1 == '.')
               {
                  insert = false;
               }
               else if((char2 == '-') && (char1 == '>'))
               {
                  insert = false;
               }
               else if((char2 == ':') && (char1 == ':'))
               {
                  insert = false;
               }

               if(insert)
               {
                  auto qual = icls->Name() + SCOPE_STR;
                  code.insert(pos, qual);
                  lexer.Initialize(code);
                  pos += qual.size();
               }
            }

            pos += id.size();
         }
      }
   }
}

//------------------------------------------------------------------------------
//
//  Replaces the identifier oldName with newName within CODE.
//
static void Rename(string& code, const string& oldName, const string& newName)
{
   Debug::ft("CodeTools.Rename");

   Lexer lexer;
   lexer.Initialize(code);

   for(auto pos = lexer.FindWord(0, oldName); pos != string::npos;
      pos = lexer.FindWord(pos, oldName))
   {
      code.replace(pos, oldName.size(), newName);
      pos += newName.size();
   }
}

//------------------------------------------------------------------------------

static word Report(c_string text, word rc = EditFailed)
{
   Debug::ft("CodeTools.Report(Editor)");

   SetExpl(text);
   return rc;
}

//------------------------------------------------------------------------------

static word Report(const std::ostringstream& stream, word rc = EditFailed)
{
   Debug::ft("CodeTools.Report(stream)");

   SetExpl(stream.str());
   return rc;
}

//------------------------------------------------------------------------------

static void SetExpl(const string& expl)
{
   Debug::ft("CodeTools.SetExpl");

   Expl_ = expl;
   if(!expl.empty() && expl.back() != CRLF) Expl_.push_back(CRLF);
}

//------------------------------------------------------------------------------

static string strCode(const string& code, size_t level)
{
   return spaces(level * IndentSize()) + code + CRLF;
}

//------------------------------------------------------------------------------
//
//  Returns TEXT, prefixed by "//  " and indented with INDENT leading spaces.
//
static string strComment(const string& text, size_t indent)
{
   auto comment = spaces(indent) + "//";
   if(!text.empty()) comment += spaces(2) + text;
   comment.push_back(CRLF);
   return comment;
}

//------------------------------------------------------------------------------
//
//  Invoked when fixing a warning still needs to be implemented.  This
//  normally doesn't occur because WarningAttrs.fixable should be false.
//
static word Unimplemented()
{
   Debug::ft("CodeTools.Unimplemented");

   return Report(NotImplemented);
}

//==============================================================================

Editor::Editor() :
   sorted_(false),
   aliased_(false),
   lastErasePos_(string::npos),
   lastEraseSize_(0)
{
   Debug::ft("Editor.ctor");
}

//------------------------------------------------------------------------------

bool Editor::AdjustHorizontally(size_t pos, size_t len, const string& spacing)
{
   Debug::ft("Editor.AdjustHorizontally");

   auto changed = false;
   auto prev = pos - 1;
   auto next = pos + len;

   if(spacing[0] == Spacing::NoGap)
   {
      auto info = GetLineInfo(pos);
      auto begin = LineRfindNonBlank(prev);
      if(int(begin) < info->depth) begin = info->depth;

      if(begin < prev)
      {
         auto count = prev - begin;
         Erase(begin + 1, count);
         next -= count;
         changed = true;
      }
   }
   else if(spacing[0] == Spacing::Gap)
   {
      if(!IsBlank(code_[prev]))
      {
         Insert(pos, SPACE_STR);
         ++next;
         changed = true;
      }
   }

   if(spacing[1] == Spacing::NoGap)
   {
      auto end = LineFindNonBlank(next);
      if(end > next)
      {
         Erase(next, end - next);
         changed = true;
      }
   }
   else if(spacing[1] == Spacing::Gap)
   {
      if(!IsBlank(code_[next]))
      {
         Insert(next, SPACE_STR);
         changed = true;
      }
   }

   return changed;
}

//------------------------------------------------------------------------------

word Editor::AdjustIndentation(const CodeWarning& log)
{
   Debug::ft("Editor.AdjustIndentation");

   //  Adjust the code's indentation to match its lexical depth.
   //
   auto pos = log.loc_.GetPos();
   return Indent(pos);
}

//------------------------------------------------------------------------------

word Editor::AdjustOperator(const CodeWarning& log)
{
   Debug::ft("Editor.AdjustOperator");

   auto oper = static_cast<const Operation*>(log.item_);
   const auto& attrs = CxxOp::Attrs[oper->Op()];

   if(AdjustHorizontally(oper->GetPos(), attrs.symbol.size(), attrs.spacing))
      return Changed(oper->GetPos());
   return NotFound("operator adjustment");
}

//------------------------------------------------------------------------------

word Editor::AdjustPunctuation(const CodeWarning& log)
{
   Debug::ft("Editor.AdjustPunctuation");

   if(log.info_.size() != 2) return NotFound("log information");
   if(AdjustHorizontally(log.Pos(), 1, log.info_)) return Changed(log.Pos());
   return NotFound("punctuation adjustment");
}

//------------------------------------------------------------------------------

word Editor::AdjustTags(const CodeWarning& log)
{
   Debug::ft("Editor.AdjustTags");

   //  A pointer tag should not be preceded by a space.  It either adheres to
   //  its type or "const".  The same is true for a reference tag, which can
   //  also adhere to a pointer tag.  Even if there is more than one detached
   //  pointer tag, only one log is generated, so fix them all.
   //
   auto tag = (log.warning_ == PtrTagDetached ? '*' : '&');
   auto stop = CurrEnd(log.Pos());
   auto changed = false;

   for(auto pos = code_.find(tag, log.Pos()); pos < stop;
      pos = code_.find(tag, pos + 1))
   {
      if(IsBlank(code_[pos - 1]))
      {
         auto prev = RfindNonBlank(pos - 1);
         auto count = pos - prev - 1;
         Erase(prev + 1, count);
         pos -= count;

         //  If the character after the tag is the beginning of an identifier,
         //  insert a space.
         //
         if(ValidFirstChars.find(code_[pos + 1]) != string::npos)
         {
            Insert(pos + 1, SPACE_STR);
         }

         changed = true;
         break;
      }
   }

   if(changed) return Changed(log.Pos());

   string target = "Detached ";
   target.push_back(tag);
   target.push_back(SPACE);
   return NotFound(target.c_str());
}

//------------------------------------------------------------------------------

word Editor::AdjustVertically()
{
   Debug::ft("Editor.AdjustVertically");

   auto done = false;

   for(auto n = 1; !done && (n <= 16); ++n)
   {
      auto actions = CheckVerticalSpacing();
      done = true;

      for(size_t pos = 0, i = 0; pos != string::npos; ++i)
      {
         switch(actions[i])
         {
         case LineOK:
            pos = NextBegin(pos);
            break;

         case InsertBlank:
            InsertLine(pos, EMPTY_STR);
            pos = NextBegin(pos);
            pos = NextBegin(pos);
            done = false;
            break;

         case ChangeToEmptyComment:
            Insert(pos, COMMENT_STR);
            pos = NextBegin(pos);
            done = false;
            break;

         case DeleteLine:
            EraseLine(pos);
            done = false;
            break;
         }
      }
   }

   return EditSucceeded;
}

//------------------------------------------------------------------------------

word Editor::AppendEndline()
{
   Debug::ft("Editor.AppendEndline");

   if(code_.back() != CRLF)
   {
      code_.push_back(CRLF);
   }

   return Changed(code_.size() - 1);
}

//------------------------------------------------------------------------------

word Editor::ChangeAccess(const CodeWarning& log, Cxx::Access acc)
{
   Debug::ft("Editor.ChangeAccess(log)");

   ItemDeclAttrs attrs(log.item_);
   attrs.access_ = acc;
   return ChangeAccess(log.item_, attrs);
}

//------------------------------------------------------------------------------

word Editor::ChangeAccess(CxxToken* item, ItemDeclAttrs& attrs)
{
   Debug::ft("Editor.ChangeAccess(item)");

   //  Move the item's declaration and update its access control.  Its
   //  existing comment, if any, will also move, so don't insert one.
   //
   string code;
   auto from = CutCode(item, code);
   auto rc = FindItemDeclLoc(item->GetClass(), item->Name(), attrs);

   if(rc != EditContinue)
   {
      Paste(from, code, from);
      return Report(UnspecifiedFailure, rc);
   }

   attrs.comment_ = false;
   InsertBelowItemDecl(attrs);
   Paste(attrs.pos_, code, from);
   InsertAboveItemDecl(attrs, EMPTY_STR);
   item->SetAccess(attrs.access_);
   return Changed(item->GetPos());
}

//------------------------------------------------------------------------------

word Editor::ChangeAssignmentToCtorCall(const CodeWarning& log)
{
   Debug::ft("Editor.ChangeAssignmentToCtorCall");

   //  Change ["const"] <type> <name> = <class> "(" [<args>] ");"
   //      to ["const"] <class> <name> "(" [<args>] ");"
   //
   //  Start by erasing ["const"] <type>, leaving a space before <name>.
   //
   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound("Initialization statement");
   auto first = LineFindFirst(begin);
   if(first == string::npos) return NotFound("Start of code");
   auto name = FindWord(first, log.item_->Name());
   if(name == string::npos) return NotFound("Variable name");
   Erase(first, name - first - 1);

   //  Erase <class>.
   //
   auto eq = FindFirstOf("=", first);
   if(eq == string::npos) return NotFound("Assignment operator");
   auto cbegin = FindNonBlank(eq + 1);
   if(cbegin == string::npos) return NotFound("Start of class name");
   auto lpar = FindFirstOf("(", eq);
   if(lpar == string::npos) return NotFound("Left parenthesis");
   auto cend = RfindNonBlank(lpar - 1);
   if(cend == string::npos) return NotFound("End of class name");
   auto cname = code_.substr(cbegin, cend - cbegin + 1);
   Erase(cbegin, cend - cbegin + 1);

   //  Paste <class> before <name> and make it const if necessary.
   //
   Paste(first, cname, cbegin);
   if(log.item_->IsConst()) first = Insert(first, "const ");

   //  Remove the '=' and the spaces around it.
   //
   eq = FindFirstOf("=", first);
   if(eq == string::npos) return NotFound("Assignment operator");
   auto left = RfindNonBlank(eq - 1);
   auto right = FindNonBlank(eq + 1);
   Erase(left + 1, right - left - 1);

   //  If there are no arguments, remove the parentheses.
   //
   lpar = FindFirstOf("(", left);
   if(lpar == string::npos) return NotFound("Left parenthesis");
   auto rpar = FindClosing('(', ')', lpar + 1);
   if(rpar == string::npos) return NotFound("Right parenthesis");
   if(code_.find_first_not_of(WhitespaceChars, lpar + 1) == rpar)
   {
      Erase(lpar, rpar - lpar + 1);
   }

   //  If the code spanned two lines, it may be possible to remove the
   //  line break.
   //
   auto semi = FindFirstOf(";", lpar - 1);
   if(!OnSameLine(begin, semi)) EraseLineBreak(begin);
   auto func = log.item_->GetScope()->GetFunction();
   ReplaceImpl(func);
   return Changed(begin);
}

//------------------------------------------------------------------------------

word Editor::ChangeAuto(const CodeWarning& log)
{
   Debug::ft("Editor.ChangeAuto");

   //  Add a & tag to the auto variable and/or tag it as const.
   //
   auto begin = CurrBegin(log.Pos());
   auto cpos = FindWord(begin, CONST_STR);
   auto apos = FindWord(begin, AUTO_STR);

   if(apos == string::npos) return NotFound("auto");

   if(log.warning_ != AutoShouldBeConst)
   {
      Insert(apos + strlen(AUTO_STR), "&");
   }

   if(cpos > apos)
   {
      switch(log.warning_)
      {
      case AutoCopiesConstReference:
      case AutoCopiesConstObject:
      case AutoShouldBeConst:
         Insert(apos, "const ");
      }
   }

   return Changed(apos);
}

//------------------------------------------------------------------------------

word Editor::ChangeCast(const CodeWarning& log)
{
   Debug::ft("Editor.ChangeCast");

   //  Replace the cast operator with one of lesser severity.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::ChangeClassToNamespace(const CodeWarning& log)
{
   Debug::ft("Editor.ChangeClassToNamespace");

   //  Replace "class" with "namespace" and "static" with "extern" (for data)
   //  or nothing (for functions).  Delete things that are no longer needed:
   //  base class, access controls, special member functions, and closing ';'.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::ChangeClassToStruct(const CodeWarning& log)
{
   Debug::ft("Editor.ChangeClassToStruct");

   //  Start by changing the class's forward declarations.
   //
   ChangeForwards(log.item_, Cxx::ClassType, Cxx::StructType);

   //  Look for the class's name and then back up to "class".
   //
   auto pos = Rfind(log.item_->GetPos(), CLASS_STR);
   if(pos == string::npos) return NotFound(CLASS_STR, true);
   Replace(pos, strlen(CLASS_STR), STRUCT_STR);

   //  If the class began with a "public:" access control, erase it.
   //
   auto left = Find(pos, "{");
   if(left == string::npos) return NotFound("Left brace");
   auto access = FindWord(left + 1, PUBLIC_STR);
   if(access != string::npos)
   {
      auto colon = FindNonBlank(access + strlen(PUBLIC_STR));
      Erase(colon, 1);
      Erase(access, strlen(PUBLIC_STR));
      if(IsBlankLine(access)) EraseLine(access);
   }

   static_cast<Class*>(log.item_)->SetClassTag(Cxx::StructType);
   return Changed(pos);
}

//------------------------------------------------------------------------------

word Editor::Changed()
{
   Debug::ft("Editor.Changed");

   Editors_.insert(this);
   return EditSucceeded;
}

//------------------------------------------------------------------------------

word Editor::Changed(size_t pos)
{
   Debug::ft("Editor.Changed(pos)");

   auto code = GetCode(pos);
   SetExpl(IsBlankLine(pos) ? EMPTY_STR : code);
   Editors_.insert(this);
   return EditSucceeded;
}

//------------------------------------------------------------------------------

void Editor::ChangeForwards
   (const CxxToken* item, Cxx::ClassTag from, Cxx::ClassTag to)
{
   Debug::ft("Editor.ChangeForwards");

   auto prev = (from == Cxx::ClassType ? CLASS_STR : STRUCT_STR);
   auto next = (from == Cxx::ClassType ? CLASS_STR : STRUCT_STR);
   SymbolVector forwards;
   auto syms = Singleton<CxxSymbols>::Instance();

   syms->FindItems(item->Name(), CLASS_FORWS, forwards);

   for(auto f = forwards.cbegin(); f != forwards.cend(); ++f)
   {
      if(!(*f)->IsInternal() && ((*f)->Referent() == item))
      {
         auto& editor = (*f)->GetFile()->GetEditor();
         auto pos = (*f)->GetPos();
         auto cpos = editor.Find(pos, prev);
         if(cpos == string::npos) continue;
         editor.Replace(pos, strlen(prev), next);
         static_cast<Forward*>(*f)->SetClassTag(to);
      }
   }
}

//------------------------------------------------------------------------------

word Editor::ChangeFunctionToMember(const Function* func, word offset)
{
   Debug::ft("Editor.ChangeFunctionToMember");

   //  o Declare the function in the class associated with the argument at
   //    OFFSET, removing that argument.
   //  o Define the function in the correct location, changing the argument
   //    at OFFSET to "this".
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::ChangeInvokerToMember(const Function* func, word offset)
{
   Debug::ft("Editor.ChangeInvokerToMember");

   //  Change invokers of this function to invoke it through the argument at
   //  OFFSET instead of directly.  An invoker may need to #include the class's
   //  header.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::ChangeMemberToFree(const CodeWarning& log)
{
   Debug::ft("Editor.ChangeMemberToFree(log)");

   //  This removes a member (static data or a function) from a class and adds
   //  it to a namespace.  This is supported if the member is used in a single
   //  .cpp, which is provided in the log's info_ field.
   //
   auto decl = static_cast<CxxScope*>(log.item_);
   auto defn = decl->GetMate();

   if(defn == nullptr)
   {
      //  LOG shouldn't have been generated in this case.
      //
      return Report("This is not supported unless a .cpp defines the item.");
   }

   auto& editor = defn->GetFile()->GetEditor();
   return editor.ChangeMemberToFree(decl);
}

//------------------------------------------------------------------------------

word Editor::ChangeMemberToFree(CxxScope* decl)
{
   Debug::ft("Editor.ChangeMemberToFree(decl)");

   const auto& oldName = decl->Name();
   auto newName = GetItemName(decl, file_);

   if(newName.empty())
   {
      return Report("This warning will be skipped.");
   }

   //  Get the code for the item's definition and modify it as follows:
   //  o Remove its class qualifier.
   //  o Prefix the "static" tag.
   //  o Prefix the declaration's comment, if any.
   //  o Remove the "const" tag from a const member function.
   //
   auto defn = decl->GetMate();
   CxxToken* ftarg = nullptr;
   auto code = GetDefnCode(defn, ftarg);
   auto cls = decl->GetClass();
   auto qual = cls->Name() + SCOPE_STR;
   auto lpar = code.find('(');
   auto pos = code.rfind(qual, lpar);
   if(pos == string::npos) return NotFound("Definition's class qualifier");
   code.erase(pos, qual.size());
   code.insert(0, string(STATIC_STR) + SPACE);

   auto comment = GetComment(decl, true);
   if(!comment.empty()) code.insert(0, comment);

   auto isFunc = (decl->Type() == Cxx::Function);
   if(isFunc)
   {
      if(decl->IsConst())
      {
         pos = code.find('{');
         if(pos == string::npos) return NotFound("Function's left brace");
         pos = code.rfind(CONST_STR, pos);
         if(pos == string::npos) return NotFound("Function const tag");
         code.erase(pos, strlen(CONST_STR));
         if(code[pos - 1] == CRLF) code.erase(pos - 1, 1);
      }
   }

   code.push_back(CRLF);

   //  The names of any public members must now be qualified.
   //
   QualifyClassItems(decl, code);

   //  If the item's name is changing, replace its occurrences.
   //
   if(newName != oldName)
   {
      CodeTools::Rename(code, oldName, newName);
   }

   //  Find the range in which the new declaration can be placed and the
   //  references to it.
   //
   size_t min, max;
   auto items = FindDeclRange(decl, min, max);
   if(min > max)
   {
      return NotFound("Position for new static declaration");
   }

   //  Before deleting the member's current declaration and definition,
   //  find the namespace to which it belongs and the references to it.
   //
   auto space = decl->GetSpace();
   auto refs = decl->GetNonLocalRefs();
   EraseItem(defn);
   pos = lastErasePos_;
   UpdateAfterErase(min);
   UpdateAfterErase(max);
   EraseItem(decl);

   //  Finalize the position of the new declaration.
   //
   ItemDefnAttrs attrs;
   if(isFunc) attrs.role_ = FuncOther;
   FindFreeItemPos(space, newName, pos, min, max, attrs);
   pos = attrs.pos_;

   if(pos == string::npos)
   {
      return NotFound("Position for new static declaration");
   }

   //  If the item is commented, ensure that it is offset.
   //
   if(!comment.empty())
   {
      if(attrs.offsets_.above_ == OffsetNone)
         attrs.offsets_.above_ = OffsetBlank;
      if(attrs.offsets_.below_ == OffsetNone)
         attrs.offsets_.below_ = OffsetBlank;
   }

   InsertBelowItemDefn(attrs);
   auto dest = Insert(pos, code);

   while(!items.empty())
   {
      auto item = items.back();
      items.pop_back();
      InsertLine(dest, EMPTY_STR);
      MoveItem(item, dest, nullptr);
   }

   InsertAboveItemDefn(attrs);

   //  Compile the declaration and find the C++ item that was created for it.
   //
   pos = code_.find(code, dest);
   pos = Find(pos, STATIC_STR);

   auto item = static_cast<CxxScope*>(ParseFileItem(pos, space));
   if(item == nullptr)
   {
      return Report("Parsing of new declaration failed.");
   }

   //  For each reference to the previous item, update the name if it changed,
   //  qualify the name if an overloaded function with the same name exists in
   //  the invoking class, remove previous qualified names, and update the
   //  referent.
   //
   for(auto r = refs.begin(); r != refs.end(); ++r)
   {
      if((*r)->Type() == Cxx::TypeName)
      {
         auto tname = static_cast<const TypeName*>(*r);
         auto qname = tname->GetQualName();
         if(newName != oldName) qname->Rename(newName);
         auto ovld = QualifyOverload(qname);
         size_t count = (ovld ? 2 : 1);
         size_t index = (ovld ? 1 : 0);
         while(qname->Size() > count) EraseItem(qname->At(index));
         qname->SetReferent(item, nullptr);
         qname->UpdateXref(false);
      }
   }

   //  In a function definition, replace the class name with the namespace in
   //  the Debug::ft argument.
   //
   if(ftarg != nullptr)
   {
      UpdateDebugFt(static_cast<Function*>(item));
   }

   if(item != nullptr) pos = item->GetPos();
   return Changed(pos);
}

//------------------------------------------------------------------------------

word Editor::ChangeOperator(const CodeWarning& log)
{
   Debug::ft("Editor.ChangeOperator");

   //  This fixes two different warnings:
   //  o StaticFunctionViaMember: change . or -> to :: and what precedes
   //    the operator to the name of the function's class.
   //  o BitwiseOperatorOnBoolean: replace | with || or & with &&.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::ChangeSpecialFunction(const CodeWarning& log)
{
   Debug::ft("Editor.ChangeSpecialFunction");

   //  This is logged on a class when the function doesn't yet exist.
   //
   if(log.item_->Type() == Cxx::Class)
   {
      return InsertSpecialFunctions(log.item_);
   }

   //  The function already exists, so its access control needs to change.
   //
   auto func = static_cast<const Function*>(log.item_);
   auto cls = func->GetClass();
   ItemDeclAttrs attrs(func);
   word rc = EditFailed;

   switch(log.warning_)
   {
   case PublicConstructor:
      if(cls->WasCreated(false)) return Report(ClassInstantiated);
      attrs.access_ = Cxx::Protected;
      break;

   case NonVirtualDestructor:
      switch(ChooseDtorAttributes(attrs))
      {
      case EditCompleted:
         return Report("Destructor remains unchanged.", EditCompleted);
      case EditFailed:
         return Report(FixSkipped);
      }

      if(attrs.virt_ != func->IsVirtual())
      {
         if(attrs.virt_)
            rc = TagAsVirtual(log);
         else
            rc = EraseVirtualTag(log);
         if(rc != EditSucceeded) return rc;
      }
      break;

   case ConstructorNotPrivate:
   case DestructorNotPrivate:
      attrs.access_ = Cxx::Private;
      break;
   default:
      return Report("Internal error: unexpected warning type.");
   }

   return ChangeAccess(log.item_, attrs);
}

//------------------------------------------------------------------------------

word Editor::ChangeStructToClass(const CodeWarning& log)
{
   Debug::ft("Editor.ChangeStructToClass");

   //  Start by changing the struct's forward declarations.
   //
   ChangeForwards(log.item_, Cxx::StructType, Cxx::ClassType);

   //  Look for the struct's name and then back up to "struct".
   //
   auto pos = Rfind(log.item_->GetPos(), STRUCT_STR);
   if(pos == string::npos) return NotFound(STRUCT_STR, true);
   Replace(pos, strlen(STRUCT_STR), CLASS_STR);

   //  Unless the struct began with a "public:" access control, insert one.
   //
   auto left = Find(pos, "{");
   if(left == string::npos) return NotFound("Left brace");
   auto access = FindWord(left + 1, PUBLIC_STR);
   if(access == string::npos)
   {
      string control(PUBLIC_STR);
      control.push_back(':');
      InsertLine(NextBegin(left), control);
   }

   static_cast<Class*>(log.item_)->SetClassTag(Cxx::ClassType);
   return Changed(pos);
}

//------------------------------------------------------------------------------

fn_name Editor_CodeBegin = "Editor.CodeBegin";

size_t Editor::CodeBegin() const
{
   Debug::ft(Editor_CodeBegin);

   size_t pos = string::npos;
   const auto& items = file_->Items();

   for(auto i = items.cbegin(); i != items.cend(); ++i)
   {
      if((*i)->IsInternal()) continue;

      switch((*i)->Type())
      {
      case Cxx::Class:
      case Cxx::Data:
      case Cxx::Enum:
      case Cxx::Function:
      case Cxx::Typedef:
         pos = (*i)->GetPos();
      }

      if(pos != string::npos) break;
   }

   auto ns = false;

   for(pos = PrevBegin(pos); pos != 0; pos = PrevBegin(pos))
   {
      auto type = PosToType(pos);

      if(!LineTypeAttr::Attrs[type].isCode && (type != FileComment))
      {
         //  Keep moving up the file.  The idea is to stop at an
         //  #include, forward declaration, or using statement.
         //
         continue;
      }

      switch(type)
      {
      case OpenBrace:
         //
         //  This should be the brace for a namespace enclosure because
         //  we started going up the file from the first code item that
         //  could be followed by one (class, enum, or function).
         //
         ns = true;
         break;

      case CodeLine:
         //
         //  If we saw an open brace, this should be a namespace enclosure.
         //  If it is, continue to back up.  If a namespace is expected but
         //  not found, generate a log.  In this case, or when a namespace
         //  *wasn't* expected, assume that the code starts after the line
         //  that we're now on, which is probably a forward declaration.
         //
         if(ns)
         {
            if(LineFind(pos, NAMESPACE_STR) != string::npos) continue;
            Debug::SwLog(Editor_CodeBegin, "namespace expected", pos);
         }
         return NextBegin(pos);

      case AccessControl:
      case DebugFt:
      case FunctionName:
         //
         //  These shouldn't occur.
         //
         Debug::SwLog(Editor_CodeBegin, "unexpected line type", type);
         [[fallthrough]];
      case FileComment:
      case CloseBrace:
      case CloseBraceSemicolon:
      case IncludeDirective:
      case HashDirective:
      case UsingStatement:
      default:
         //
         //  We're now one line above what should be the start of the
         //  file's code, plus any relevant comments that precede it.
         //
         return NextBegin(pos);
      }
   }

   return pos;
}

//------------------------------------------------------------------------------

bool Editor::CodeFollowsImmediately(size_t pos) const
{
   Debug::ft("Editor.CodeFollowsImmediately");

   //  Proceed from POS, skipping blank lines and access controls.  Return
   //  false if the next thing is executable code (this excludes braces and
   //  access controls), else return false.
   //
   pos = NextBegin(pos);
   if(pos == string::npos) return false;
   return (PosToType(pos) == CodeLine);
}

//------------------------------------------------------------------------------

bool Editor::CommentFollows(size_t pos) const
{
   Debug::ft("Editor.CommentFollows");

   pos = LineFindNext(pos);
   if(pos == string::npos) return false;
   if(CompareCode(pos, COMMENT_STR) == 0) return true;
   return (CompareCode(pos, COMMENT_BEGIN_STR) == 0);
}

//------------------------------------------------------------------------------

void Editor::Commit(CliThread& cli)
{
   Debug::ft("Editor.Commit");

   while(!Editors_.empty())
   {
      string expl;
      auto editor = Editors_.cbegin();

      while(true)
      {
         auto rc = (*editor)->Format(expl);
         *cli.obuf << spaces(2) << expl;
         expl.clear();

         if(rc == EditCompleted)
         {
            ++Commits_;
            break;
         }

         if(!cli.BoolPrompt(CommitFailedPrompt)) break;
      }

      Editors_.erase(editor);
   }
}

//------------------------------------------------------------------------------

size_t Editor::CommitCount() { return Commits_; }

//------------------------------------------------------------------------------

word Editor::ConvertTabsToBlanks()
{
   Debug::ft("Editor.ConvertTabsToBlanks");

   auto indent = IndentSize();

   //  Run through the source, looking for tabs.
   //
   for(size_t pos = code_.find(TAB); pos != string::npos;
      pos = code_.find(TAB, pos))
   {
      //  Find the start of this line.  If the tab appears in a comment,
      //  ignore it.  Otherwise determine how many spaces to insert when
      //  replacing the tab.
      //
      auto begin = CurrBegin(pos);
      auto end = LineFind(begin, COMMENT_STR);
      if(pos >= end) continue;

      auto count = (pos - begin) % indent;
      if(count == 0) count = indent;
      Replace(pos, 1, spaces(count));
      Changed();
   }

   return EditSucceeded;
}

//------------------------------------------------------------------------------

size_t Editor::CutCode(const CxxToken* item, string& code)
{
   Debug::ft("Editor.CutCode");

   code.clear();

   if(item == nullptr)
   {
      Report("Internal error: no item specified.");
      return string::npos;
   }

   size_t begin, end;
   GetSpan(item, begin, end);

   //  Extract the code bounded by [begin, end].  If BEGIN and END are not
   //  on the same line, then
   //  o if BEGIN is not at the start of its line, insert a CRLF before it
   //  o if END is not at the end of its line, insert a CRLF after it (and
   //    include it in the code) and re-indent that line
   //
   if(!OnSameLine(begin, end))
   {
      if(code_[begin - 1] != CRLF)
      {
         Insert(begin, CRLF_STR);
         ++begin;
         ++end;
      }

      if(code_[end] != CRLF)
      {
         auto indent = CRLF_STR + spaces(end - CurrBegin(end));
         Insert(end + 1, indent);
         ++end;
      }
   }

   code = code_.substr(begin, end - begin + 1);
   Erase(begin, end - begin + 1);
   return begin;
}

//------------------------------------------------------------------------------

word Editor::DeleteSpecialFunction(const CodeWarning& log)
{
   Debug::ft("Editor.DeleteSpecialFunction");

   //  This is logged on a class when the function doesn't yet exist.
   //
   if(log.item_->Type() == Cxx::Class)
   {
      return InsertSpecialFunctions(log.item_);
   }

   switch(log.warning_)
   {
   case CopyCtorNotDeleted:
   case CopyOperNotDeleted:
   case CtorCouldBeDeleted:
      break;
   default:
      return Report(NotImplemented);
   }

   //  If the function has a definition, erase it.
   //
   auto decl = static_cast<Function*>(log.item_);
   auto defn = decl->GetDefn();
   if(defn != decl)
   {
      if(EraseItem(defn) == EditFailed) return EditFailed;
   }

   //  Find the function declaration and insert " = delete" before its
   //  closing semicolon.
   //
   size_t begin, end;
   if(!decl->GetSpan2(begin, end)) return NotFound("Function declaration");
   auto pos = CurrBegin(end);
   auto semi = code_.find(';', pos);
   if(semi == string::npos) return NotFound("Function semicolon");
   Insert(semi, " = delete");
   decl->SetDeleted();

   //  Ensure that the function's access control is public.
   //
   ItemDeclAttrs attrs(decl);
   if(attrs.access_ != Cxx::Public)
   {
      attrs.access_ = Cxx::Public;
      return ChangeAccess(log.item_, attrs);
   }

   return Changed(semi);
}

//------------------------------------------------------------------------------

word Editor::DemoteFunction(const CodeWarning& log)
{
   Debug::ft("Editor.DemoteFunction");

   //  Move the function to the derived class specified in the log.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

void Editor::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Lexer::Display(stream, prefix, options);

   stream << prefix << "file     : " <<
      (file_ != nullptr ? file_->Name() : "no file specified") << CRLF;
   stream << prefix << "sorted   : " << sorted_ << CRLF;
   stream << prefix << "aliased  : " << aliased_ << CRLF;
}

//------------------------------------------------------------------------------

bool Editor::DisplayLog(const CodeWarning& log, bool file) const
{
   Debug::ft("Editor.DisplayLog");

   if(file)
   {
      *Cli_->obuf << log.File()->Name() << ':' << CRLF;
   }

   //  Display LOG's details.
   //
   *Cli_->obuf << "  Line " << log.Line() + 1;
   if(log.offset_ > 0) *Cli_->obuf << '/' << log.offset_;
   *Cli_->obuf << ": " << Warning(log.warning_);
   if(log.HasInfoToDisplay()) *Cli_->obuf << ": " << log.info_;
   *Cli_->obuf << CRLF;

   if(log.HasCodeToDisplay())
   {
      //  Display the current version of the code associated with LOG.
      //
      *Cli_->obuf << spaces(2);
      auto code = GetCode(log.Pos());

      if(code.empty())
      {
         *Cli_->obuf << "Code not found." << CRLF;
         return false;
      }

      if(code.find_first_not_of(WhitespaceChars) == string::npos)
      {
         *Cli_->obuf << "[line contains only whitespace]" << CRLF;
         return true;
      }

      *Cli_->obuf << code;
   }

   return true;
}

//------------------------------------------------------------------------------

size_t Editor::Erase(size_t pos, size_t count)
{
   Debug::ft("Editor.Erase");

   if(count == 0) return pos;
   code_.erase(pos, count);
   UpdatePos(Erased, pos, count);
   return pos;
}

//------------------------------------------------------------------------------

word Editor::EraseAccessControl(const CodeWarning& log)
{
   Debug::ft("Editor.EraseAccessControl");

   //  The parser logs RedundantAccessControl at the position
   //  where it occurred; log.item_ is nullptr in this warning.
   //
   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound("Access control position");
   size_t len = 0;

   //  Look for the access control keyword and note its length.
   //
   auto access = LineFind(begin, PUBLIC_STR);

   while(true)
   {
      if(access != string::npos)
      {
         len = strlen(PUBLIC_STR);
         break;
      }

      access = LineFind(begin, PROTECTED_STR);

      if(access != string::npos)
      {
         len = strlen(PROTECTED_STR);
         break;
      }

      access = LineFind(begin, PRIVATE_STR);

      if(access != string::npos)
      {
         len = strlen(PRIVATE_STR);
         break;
      }

      return NotFound("Access control keyword");
   }

   //  Look for the colon that follows the keyword.
   //
   auto colon = FindNonBlank(access + len);
   if((colon == string::npos) || (code_[colon] != ':'))
      return NotFound("Colon after access control");

   //  Erase the keyword and colon.
   //
   Erase(colon, 1);
   Erase(access, len);
   return Changed(access);
}

//------------------------------------------------------------------------------

word Editor::EraseAdjacentSpaces(const CodeWarning& log)
{
   Debug::ft("Editor.EraseAdjacentSpaces");

   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound("Line with adjacent spaces");
   auto pos = LineFindFirst(begin);
   if(pos == string::npos) return NotFound("Adjacent spaces");

   //  If this line has a trailing comment that is aligned with one on the
   //  previous or the next line, keep the comments aligned by moving the
   //  erased spaces immediately to the left of the comment.
   //
   auto move = false;
   auto cpos = FindComment(pos);

   if(cpos != string::npos)
   {
      cpos -= begin;

      if(pos != begin)
      {
         auto prev = PrevBegin(begin);
         if(prev != string::npos)
         {
            move = (cpos == (FindComment(prev) - prev));
         }
      }

      if(!move)
      {
         auto next = NextBegin(begin);
         if(next != string::npos)
         {
            move = (cpos == (FindComment(next) - next));
         }
      }
   }

   //  Don't erase adjacent spaces that precede a trailing comment.
   //
   auto stop = cpos;

   if(stop != string::npos)
      while(IsBlank(code_[stop - 1])) --stop;
   else
      stop = CurrEnd(begin);

   cpos = stop;  // (comm - stop) will be number of erased spaces

   while(pos + 1 < stop)
   {
      if(IsBlank(code_[pos]) && IsBlank(code_[pos + 1]))
      {
         Erase(pos, 1);
         --stop;
      }
      else
      {
         ++pos;
      }
   }

   if(move) Insert(stop, string(cpos - stop, SPACE));
   return Changed(begin);
}

//------------------------------------------------------------------------------

word Editor::EraseArgument(const Function* func, word offset)
{
   Debug::ft("Editor.EraseArgument");

   //  In this function invocation, erase the argument at OFFSET.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

fn_name Editor_EraseAssignment = "Editor.EraseAssignment";

word Editor::EraseAssignment(const CxxToken* item)
{
   Debug::ft(Editor_EraseAssignment);

   //  ITEM should be a TypeName or QualName in an assignment statement
   //  or a constructor's member initialization.
   //
   switch(item->Type())
   {
   case Cxx::QualName:
   case Cxx::TypeName:
   case Cxx::MemberInit:
      break;
   default:
      Debug::SwLog(Editor_EraseAssignment, strClass(item, false), 0);
      return EditFailed;
   }

   //  Back up to the beginning of ITEM's statement by finding the end
   //  of the previous statement and stepping forward.
   //
   auto file = item->GetFile();
   auto& editor = file->GetEditor();
   auto pos = item->GetPos();
   auto end = editor.RfindFirstOf(pos, "{};,");
   auto begin = editor.NextPos(end + 1);
   auto statement = file->PosToItem(begin);
   if(statement == nullptr) return NotFound("Item's statement");

   string code;
   if(CutCode(statement, code) == string::npos) return EditFailed;
   statement->Delete();
   return Changed();
}

//------------------------------------------------------------------------------

word Editor::EraseBlankLine(const CodeWarning& log)
{
   Debug::ft("Editor.EraseBlankLine");

   //  Remove the specified line of code.
   //
   EraseLine(log.Pos());
   return Changed();
}

//------------------------------------------------------------------------------

word Editor::EraseCast(const CodeWarning& log)
{
   Debug::ft("Editor.EraseCast");

   //  Remove the cast operator.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::EraseClass(const CodeWarning& log)
{
   Debug::ft("Editor.EraseClass");

   //  Erase the class's definition and the definitions of its functions
   //  and static data.  Erase definitions before declarations to avoid
   //  having definitions without declarations.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::EraseConst(const CodeWarning& log)
{
   Debug::ft("Editor.EraseConst");

   //  There are two places for a redundant "const" after the typename:
   //    "const" <typename> "const" [<typetags>] "const"
   //  The log indicates the position of the redundant "const".  If there is
   //  more than one of them, each one causes a log, so we can erase them one
   //  at a time.  To ensure that the wrong "const" is not erased after other
   //  edits have occurred on this line, verify that the const is not preceded
   //  by certain punctuation.  A preceding '(' or ',' can only occur if the
   //  "const" precedes its typename, and a '*' can only occur if the "const"
   //  makes a pointer const.
   //
   for(auto pos = FindWord(log.Pos(), CONST_STR); pos != string::npos;
      pos = FindWord(pos + 1, CONST_STR))
   {
      auto prev = RfindNonBlank(pos - 1);

      //  This is the first non-blank character preceding a subsequent
      //  "const".  If it's a pointer tag, it makes the pointer const,
      //  so continue with the next "const".
      //
      switch(code_[prev])
      {
      case '*':
      case ',':
      case '(':
         continue;
      }

      //  This is the redundant const, so erase it.  Also erase a space
      //  between it and the previous non-blank character.
      //
      Erase(pos, strlen(CONST_STR));

      if(OnSameLine(prev, pos) && (pos - prev > 1))
      {
         Erase(prev + 1, 1);
      }

      return Changed(pos);
   }

   return NotFound("Redundant const");
}

//------------------------------------------------------------------------------

word Editor::EraseDefaultValue(const Function* func, word offset)
{
   Debug::ft("Editor.EraseDefaultValue");

   //  Erase this argument's default value.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::EraseEmptyNamespace(SpaceDefn* ns)
{
   Debug::ft("Editor.EraseEmptyNamespace");

   if(ns == nullptr) return EditSucceeded;
   size_t begin, left, end;
   if(!ns->GetSpan3(begin, left, end)) return EditSucceeded;
   auto pos = NextPos(left + 1);
   if(code_[pos] != '}') return EditSucceeded;
   return EraseItem(ns);
}

//------------------------------------------------------------------------------

word Editor::EraseExplicitTag(const CodeWarning& log)
{
   Debug::ft("Editor.EraseExplicitTag");

   auto exp = Rfind(log.item_->GetPos(), EXPLICIT_STR);
   if(exp == string::npos) return NotFound(EXPLICIT_STR, true);
   Erase(exp, strlen(EXPLICIT_STR) + 1);
   static_cast<Function*>(log.item_)->SetExplicit(false);
   return Changed(exp);
}

//------------------------------------------------------------------------------

word Editor::EraseForward(const CodeWarning& log)
{
   Debug::ft("Editor.EraseForward");

   //  Erasing the forward declaration may leave an empty enclosing
   //  namespace that should be deleted.
   //
   auto ns = file_->FindNamespaceDefn(log.item_);
   if(EraseItem(log.item_) == EditFailed) return EditFailed;
   return EraseEmptyNamespace(ns);
}

//------------------------------------------------------------------------------

word Editor::EraseItem(CxxToken* item)
{
   Debug::ft("Editor.EraseItem");

   auto& editor = item->GetFile()->GetEditor();

   string code;
   if(editor.CutCode(item, code) == string::npos) return EditFailed;
   item->Delete();
   return editor.Changed();
}

//------------------------------------------------------------------------------

size_t Editor::EraseLine(size_t pos)
{
   Debug::ft("Editor.EraseLine");

   auto begin = CurrBegin(pos);
   auto end = CurrEnd(pos);
   Erase(begin, end - begin + 1);
   return begin;
}

//------------------------------------------------------------------------------

bool Editor::EraseLineBreak(size_t pos)
{
   Debug::ft("Editor.EraseLineBreak(pos)");

   auto space = CheckLineMerge(GetLineNum(pos));
   if(space < 0) return false;

   //  Merge the lines after replacing or erasing CURR's endline.
   //
   auto next = NextBegin(pos);
   auto start = LineFindFirst(next);

   if(space != 0)
   {
      Replace(next - 1, 1, SPACE_STR);
      Erase(next, start - next);
   }
   else
   {
      Erase(next - 1, start - next + 1);
   }

   return true;
}

//------------------------------------------------------------------------------

word Editor::EraseLineBreak(const CodeWarning& log)
{
   Debug::ft("Editor.EraseLineBreak(log)");

   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound("Position for line break");
   auto merged = EraseLineBreak(begin);
   if(!merged) return Report("Line break was not removed.");
   return Changed(begin);
}

//------------------------------------------------------------------------------

word Editor::EraseMutableTag(const CodeWarning& log)
{
   Debug::ft("Editor.EraseMutableTag");

   //  Find the line on which the data's type appears, and erase the
   //  "mutable" before that type.
   //
   auto tag = Rfind(log.item_->GetTypeSpec()->GetPos(), MUTABLE_STR);
   if(tag == string::npos) return NotFound(MUTABLE_STR, true);
   Erase(tag, strlen(MUTABLE_STR) + 1);
   static_cast<ClassData*>(log.item_)->SetMutable(false);
   return Changed(tag);
}

//------------------------------------------------------------------------------

word Editor::EraseNoexceptTag(Function* func)
{
   Debug::ft("Editor.EraseNoexceptTag");

   //  Look for "noexcept" just before the end of the function's signature.
   //
   auto endsig = FindSigEnd(func);
   if(endsig == string::npos) return NotFound("Signature end");
   endsig = Rfind(endsig, NOEXCEPT_STR);
   if(endsig == string::npos) return NotFound(NOEXCEPT_STR, true);
   size_t space = (IsFirstNonBlank(endsig) ? 0 : 1);
   Erase(endsig - space, strlen(NOEXCEPT_STR) + space);
   func->SetNoexcept(false);
   return Changed(endsig);
}

//------------------------------------------------------------------------------

word Editor::EraseOverrideTag(const CodeWarning& log)
{
   Debug::ft("Editor.EraseOverrideTag");

   //  Look for "override" just before the end of the function's signature.
   //
   auto endsig = FindSigEnd(log);
   if(endsig == string::npos) return NotFound("Signature end");
   endsig = Rfind(endsig, OVERRIDE_STR);
   if(endsig == string::npos) return NotFound(OVERRIDE_STR, true);
   size_t space = (IsFirstNonBlank(endsig) ? 0 : 1);
   Erase(endsig - space, strlen(OVERRIDE_STR) + space);
   static_cast<Function*>(log.item_)->SetOverride(false);
   return Changed(endsig);
}

//------------------------------------------------------------------------------

word Editor::EraseParameter(const Function* func, word offset)
{
   Debug::ft("Editor.EraseParameter");

   //  Erase the argument at OFFSET in this function definition or declaration.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::EraseScope(const CodeWarning& log)
{
   Debug::ft("Editor.EraseScope");

   auto qname = static_cast<const QualName*>(log.item_);
   auto begin = qname->GetPos();
   auto op = code_.find(SCOPE_STR, begin);
   if(op == string::npos) return NotFound("Scope resolution operator");
   Erase(begin, op - begin + 2);
   qname->First()->Delete();
   return Changed(begin);
}

//------------------------------------------------------------------------------

word Editor::EraseSemicolon(const CodeWarning& log)
{
   Debug::ft("Editor.EraseSemicolon");

   //  The parser logs a redundant semicolon that follows the closing '}'
   //  of a function definition or namespace enclosure.
   //
   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound("Position of semicolon");
   auto semi = FindFirstOf(";", begin);
   if(semi == string::npos) return NotFound("Semicolon");
   auto brace = RfindNonBlank(semi - 1);
   if(brace == string::npos) return NotFound("Right brace");
   if(code_[brace] != '}') return NotFound("Right brace");
   Erase(semi, 1);
   return Changed(semi);
}

//------------------------------------------------------------------------------

word Editor::EraseTrailingBlanks()
{
   Debug::ft("Editor.EraseTrailingBlanks");

   for(size_t begin = 0; begin != string::npos; begin = NextBegin(begin))
   {
      auto end = CurrEnd(begin);
      if(begin == end) continue;

      auto pos = end - 1;
      while(IsBlank(code_[pos]) && (pos >= begin)) --pos;

      if(pos < end - 1)
      {
         Erase(pos + 1, end - pos - 1);
         Changed();
      }
   }

   return EditSucceeded;
}

//------------------------------------------------------------------------------

word Editor::EraseVirtualTag(const CodeWarning& log)
{
   Debug::ft("Editor.EraseVirtualTag");

   //  Look for "virtual" just before the function's return type.
   //
   auto virt = LineRfind(log.item_->GetTypeSpec()->GetPos(), VIRTUAL_STR);
   if(virt == string::npos) return NotFound(VIRTUAL_STR, true);
   Erase(virt, strlen(VIRTUAL_STR) + 1);
   static_cast<Function*>(log.item_)->SetVirtual(false);
   return Changed(virt);
}

//------------------------------------------------------------------------------

word Editor::EraseVoidArgument(const CodeWarning& log)
{
   Debug::ft("Editor.EraseVoidArgument");

   //  The function might return "void", so the second occurrence of "void"
   //  could be the argument.  Erase it, leaving only the parentheses.
   //
   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound("Position of void argument");

   for(auto arg = FindWord(begin, VOID_STR); arg != string::npos;
      arg = FindWord(arg + 1, VOID_STR))
   {
      auto lpar = RfindNonBlank(arg - 1);
      if(lpar == string::npos) continue;
      if(code_[lpar] != '(') continue;
      auto rpar = FindNonBlank(arg + strlen(VOID_STR));
      if(rpar == string::npos) break;
      if(code_[rpar] != ')') continue;
      if(OnSameLine(arg, lpar) && OnSameLine(arg, rpar))
      {
         Erase(lpar + 1, rpar - lpar - 1);
         return Changed(lpar);
      }
      Erase(arg, strlen(VOID_STR));
      return Changed(arg);
   }

   return NotFound(VOID_STR, true);
}

//------------------------------------------------------------------------------

size_t Editor::FindArgsEnd(const Function* func) const
{
   Debug::ft("Editor.FindArgsEnd");

   auto name = func->GetPos();
   if(name == string::npos) return string::npos;
   auto lpar = FindFirstOf("(", name);
   if(lpar == string::npos) return string::npos;
   auto rpar = FindClosing('(', ')', lpar + 1);
   return rpar;
}

//------------------------------------------------------------------------------

CxxItemVector Editor::FindDeclRange
   (CxxScope* decl, size_t& begin, size_t& end) const
{
   Debug::ft("Editor.FindDeclRange");

   auto refs = decl->GetNonLocalRefs();
   end = file_->FindFirstReference(refs);

   //  Exclude any items declared immediately above the function and used by
   //  it, such as its fn_name.  These will be moved with the function.
   //
   auto usages = GetItemUsages(decl, false);
   auto defn = decl->GetMate();
   auto items = GetItemsForDefn(defn);

   for(auto i = items.cbegin(); i != items.cend(); ++i)
   {
      usages.directs.erase(static_cast<CxxNamed*>(*i));
      usages.indirects.erase(static_cast<CxxNamed*>(*i));
   }

   begin = file_->FindLastUsage(usages.directs);
   auto max = file_->FindLastUsage(usages.indirects);
   if(max > begin) begin = max;

   return items;
}

//------------------------------------------------------------------------------

void Editor::FindFreeItemPos(const Namespace* space, const string& name,
   size_t pos, size_t min, size_t max, ItemDefnAttrs& attrs) const
{
   Debug::ft("Editor.FindFreeItemPos");

   auto isFunc = (attrs.role_ == FuncOther);

   //  If possible, place data where its previous definition was located.
   //
   if(!isFunc)
   {
      if((pos >= min) && (pos <= max))
      {
         attrs.pos_ = pos;
         return;
      }
   }

   //  Place the item between BEGIN and END as long as it lies within the
   //  same namespace.
   //
   attrs.pos_ = string::npos;
   attrs.offsets_.below_ = OffsetRule;
   const auto& items = file_->Items();

   for(auto i = items.cbegin(); i != items.cend(); ++i)
   {
      size_t begin, left, end;

      //  Keep going if we haven't reached the desired namespace or MIN.
      //  Return if we've passed MAX.
      //
      if((*i)->IsInternal()) continue;

      auto currSpace = (*i)->GetSpace();
      if(currSpace != space) continue;

      auto type = (*i)->Type();
      auto currPos = (*i)->GetPos();

      if(currPos < min)
      {
         if(type == Cxx::Class)
         {
            //  The item must go after this class definition.  MIN was set
            //  based on the location of a function in this class that the
            //  item uses, but bypass the whole class.
            //
            if(!(*i)->GetSpan3(begin, left, end)) return;
            auto next = NextBegin(end);
            if(next > min) min = next;
            attrs.offsets_.below_ = OffsetNone;
            attrs.offsets_.above_ = OffsetRule;
         }

         continue;
      }

      if(attrs.pos_ == string::npos) attrs.pos_ = min;
      if(currPos > max) return;

      if(type == Cxx::SpaceDefn)
      {
         //  Update the insertion position to the line following the
         //  namespace definition's left brace.
         //
         if(!(*i)->GetSpan3(begin, left, end)) return;
         attrs.pos_ = NextBegin(left);
         continue;
      }

      if(!isFunc)
      {
         //  Free data goes after existing free data.
         //
         if(type != Cxx::Data) return;
         auto data = static_cast<const Data*>(*i);
         if(data->GetClass() != nullptr) return;
         attrs.offsets_.below_ = OffsetNone;
      }
      else
      {
         //  A free function goes with free function definitions in
         //  alphabetical order.  If it will appear after another item,
         //  put its rule above it.
         //
         switch(type)
         {
         case Cxx::Function:
         {
            auto func = static_cast<const Function*>(*i);
            if(func->GetClass() != nullptr) return;
            if(func->GetDefn() == func)
            {
               if(strCompare(func->Name(), name) > 0) return;
            }
            break;
         }

         case Cxx::Data:
         {
            //  Skip over an fn_name.  It belongs to the next function, and
            //  we wouldn't want to insert the new function between them.
            //
            auto data = static_cast<const Data*>(*i);
            if(data->GetTypeSpec()->Name() == "fn_name") continue;
            break;
         }

         case Cxx::Class:
            //
            //  Put the function above a class that is private to the .cpp.
            //
            return;
         }

         attrs.offsets_.below_ = OffsetNone;
         attrs.offsets_.above_ = OffsetRule;
      }

      //  Update the insertion position to the line after the current item.
      //
      if(!(*i)->GetSpan2(begin, end)) return;
      attrs.pos_ = NextBegin(end);
   }
}

//------------------------------------------------------------------------------

word Editor::FindFuncDefnLoc(const CodeFile* file, const CxxArea* area,
   const string& name, ItemDefnAttrs& attrs) const
{
   Debug::ft("Editor.FindFuncDefnLoc(name)");

   //  Look at all the functions that are defined in this file and that belong
   //  to AREA.
   //
   auto defns = file->GetFuncDefnsToSort();
   const Function* prev = nullptr;
   const Function* next = nullptr;
   auto reached = false;

   for(auto f = defns.cbegin(); f != defns.cend(); ++f)
   {
      //  If the current function is in another area, insert the new function
      //  o immediately before it, if the new function's class has been reached
      //  o somewhere after it, if the new function's class has not been reached
      //
      if((*f)->GetArea() != area)
      {
         if(reached)
         {
            next = (*f)->GetDefn();
            break;
         }

         prev = (*f)->GetDefn();
         continue;
      }

      //  The current function is in the same area.  Insert the new function
      //  in the cardinal order defined by FunctionRole.  When the roles match,
      //  insert the new function alphabetically.
      //
      reached = true;
      auto currRole = (*f)->FuncRole();

      if(attrs.role_ < currRole)
      {
         next = (*f)->GetDefn();
         break;
      }
      else if(attrs.role_ > currRole)
      {
         prev = (*f)->GetDefn();
         continue;
      }

      const auto& currName = (*f)->Name();
      auto sort = currName.compare(name);

      if(sort > 0)
      {
         next = (*f)->GetDefn();
         break;
      }
      else if(sort < 0)
      {
         prev = (*f)->GetDefn();
         continue;
      }
      else
      {
         return Report("A definition for this function already exists.");
      }
   }

   //  We now know the functions between which the new function should be
   //  inserted (PREV and NEXT), so find its precise insertion location
   //  and attributes.
   //
   return UpdateItemDefnLoc(prev, nullptr, next, attrs);
}

//------------------------------------------------------------------------------

word Editor::FindItemDeclLoc
   (const Class* cls, const string& name, ItemDeclAttrs& attrs) const
{
   Debug::ft("Editor.FindItemDeclLoc");

   auto where = attrs.CalcDeclOrder();
   auto items = cls->Items();
   const CxxToken* prev = nullptr;
   const CxxToken* next = nullptr;

   for(auto i = items.cbegin(); i != items.cend(); ++i)
   {
      ItemDeclAttrs currAttrs(*i);
      auto order = currAttrs.CalcDeclOrder();

      if(where < order)
      {
         next = *i;
         break;
      }
      else if(where == order)
      {
         if(strCompare((*i)->Name(), name) > 0)
         {
            next = *i;
            break;
         }
      }

      prev = *i;
   }

   //  We now know the items between which the new item should be inserted
   //  (PREV and NEXT), so update its insertion attributes.
   //
   attrs.prev_ = prev;
   attrs.next_ = next;
   return UpdateItemDeclLoc(cls, attrs);
}

//------------------------------------------------------------------------------

size_t Editor::FindSigEnd(const CodeWarning& log)
{
   Debug::ft("Editor.FindSigEnd(log)");

   if(log.item_ == nullptr) return string::npos;
   if(log.item_->Type() != Cxx::Function) return string::npos;
   return FindSigEnd(static_cast<const Function*>(log.item_));
}

//------------------------------------------------------------------------------

size_t Editor::FindSigEnd(const Function* func)
{
   Debug::ft("Editor.FindSigEnd(func)");

   //  Look for the first semicolon or left brace after the function's name.
   //
   return FindFirstOf(";{", func->GetPos());
}

//------------------------------------------------------------------------------

word Editor::FindSpecialFuncDeclLoc
   (const Class* cls, ItemDeclAttrs& attrs) const
{
   Debug::ft("Editor.FindSpecialFuncDeclLoc");

   auto base = cls->IsBaseClass();
   auto sbase = cls->IsSingletonBase();
   auto inst = cls->WasCreated(false);
   auto solo = cls->IsSingleton();
   auto prompt = false;
   Function* prototype = cls->FindFuncByRole(PureDtor, false);
   if(prototype == nullptr) prototype = cls->FindFuncByRole(CopyCtor, false);
   if(prototype == nullptr) prototype = cls->FindFuncByRole(CopyOper, false);

   //  Update ATTRS based on what function is being inserted.
   //
   switch(attrs.role_)
   {
   case PureCtor:
      prototype = nullptr;

      if(solo)
         attrs.access_ = Cxx::Private;
      else if(base && !inst)
         attrs.access_ = Cxx::Protected;
      else if(!inst)
         prompt = true;
      break;

   case PureDtor:
      if(base)
         attrs.virt_ = true;
      else if(solo)
         attrs.access_ = Cxx::Private;
      break;

   case CopyCtor:
   case CopyOper:
      if(sbase || solo)
      {
         attrs.deleted_ = true;
      }
      else
      {
         if(base && !inst) attrs.access_ = Cxx::Protected;
         prompt = true;
      }
      break;
   }

   //  The user can decide to define the function as defaulted or deleted.
   //  If it will be defaulted and a related function is not trivial, ask
   //  if the user wants to create a shell for implementing the function.
   //
   if(prompt)
   {
      std::ostringstream stream;
      stream << spaces(2) << "Define the " << attrs.role_ << ": " << DefnPrompt;
      auto response = Cli_->IntPrompt(stream.str(), 0, 2);
      if(response == 0) return Report(FixSkipped);
      attrs.deleted_ = (response == 2);
   }

   if(attrs.deleted_)
   {
      attrs.access_ = Cxx::Public;
   }
   else if((prototype != nullptr) && !prototype->IsTrivial())
   {
      std::ostringstream stream;
      stream << "The " << prototype->FuncRole();
      stream << " is not trivial." << CRLF;
      stream << spaces(2) << ShellPrompt;
      attrs.shell_ = Cli_->BoolPrompt(stream.str());
   }

   return FindItemDeclLoc(cls, EMPTY_STR, attrs);
}

//------------------------------------------------------------------------------

CxxNamedSet Editor::FindUsingReferents(CxxToken* item) const
{
   Debug::ft("Editor.FindUsingReferents");

   CxxUsageSets symbols;
   CxxNamedSet refs;

   item->GetUsages(*file_, symbols);

   for(auto u = symbols.users.cbegin(); u != symbols.users.cend(); ++u)
   {
      refs.insert((*u)->DirectType());
   }

   return refs;
}

//------------------------------------------------------------------------------

word Editor::Fix(CliThread& cli, const FixOptions& opts, string& expl) const
{
   Debug::ft("Editor.Fix");

   //  If there is an #include for an external file that is not in the subs/
   //  directory, file_ will be nullptr.
   //
   if(file_ == nullptr) return 0;

   Cli_ = &cli;

   //  Run through all the warnings.
   //
   word rc = 0;
   auto reply = 'y';
   auto found = false;
   auto fixed = false;
   auto first = true;
   auto exit = false;
   const auto& warnings = file_->GetWarnings();

   for(auto item = warnings.cbegin(); item != warnings.cend(); ++item)
   {
      //  Skip this item if the user didn't include its warning type.
      //
      if((opts.warning != AllWarnings) &&
         (opts.warning != item->warning_)) continue;

      switch(FixStatus(*item))
      {
      case NotFixed:
         //
         //  This item is eligible for fixing, and note that such an item
         //  was found.
         //
         found = true;
         break;

      case Fixed:
      case Pending:
         //
         //  Before skipping over an eligible item that was already fixed,
         //  note that such an item was found.
         //
         fixed = true;
         continue;

      case Disabled:
      case Revoked:
         //
         //  This log wasn't even reported, so ignore it.
         //
         continue;

      case Deleted:
         if(opts.warning == AllWarnings) continue;
         *Cli_->obuf << ItemDeleted << CRLF;
         return EditAbort;

      case NotSupported:
      default:
         //
         //  If multiple warning types are being fixed, try the next one.  If
         //  only one warning type was selected, there will be nothing to fix,
         //  so return a value that will terminate the >fix command.
         //
         if(opts.warning == AllWarnings) continue;
         *Cli_->obuf << NotImplemented << CRLF;
         return EditAbort;
      }

      //  This item is eligible for fixing.  Display it.
      //
      if(DisplayLog(*item, first))
      {
         first = false;

         //  See if the user wishes to fix the item.  Valid responses are
         //  o 'y' = fix it, which invokes the appropriate function
         //  o 'n' = don't fix it
         //  o 's' = skip this file
         //  o 'q' = done fixing warnings
         //
         expl.clear();
         reply = 'y';

         if(opts.prompt)
         {
            std::ostringstream stream;
            stream << spaces(2) << FixPrompt;
            reply = Cli_->CharPrompt(stream.str(), YNSQChars, YNSQHelp);
         }

         switch(reply)
         {
         case 'y':
         {
            auto logs = item->LogsToFix(expl);

            if(!expl.empty())
            {
               *Cli_->obuf << spaces(2) << expl << CRLF;
               expl.clear();
            }

            for(auto log = logs.begin(); log != logs.end(); ++log)
            {
               auto& editor = (*log)->File()->GetEditor();
               rc = editor.FixLog(**log);
               ReportFix(*log, rc);
            }

            break;
         }

         case 'n':
            break;

         case 's':
         case 'q':
            exit = true;
            break;

         default:
            *Cli_->obuf << "Internal error: unknown response." << CRLF;
            return -6;
         }
      }

      Cli_->Flush();
      if(!opts.prompt) ThisThread::Pause(msecs_t(20));
      if(exit || (rc < EditFailed)) break;
   }

   if(found)
   {
      if(exit || (rc < EditFailed))
         *Cli_->obuf << "Remaining warnings skipped." << CRLF;
      else
         *Cli_->obuf << "End of warnings." << CRLF;
   }
   else if(fixed)
   {
      *Cli_->obuf << "Selected warning(s) in " << file_->Name();
      *Cli_->obuf << " previously fixed." << CRLF;
   }
   else if(!opts.multiple)
   {
      *Cli_->obuf << "No warnings that can be fixed were found." << CRLF;
   }

   //  Returning -1 or greater indicates that the next file can still be
   //  processed, so return a lower value if the user wants to quit or if
   //  a serious error occurred.  In that case, clear EXPL, since it has
   //  already been displayed when writing the last file.
   //
   Commit(*Cli_);

   if((reply == 'q') && (rc >= EditFailed))
   {
      expl.clear();
      rc = -2;
   }

   return rc;
}

//------------------------------------------------------------------------------

word Editor::FixData(Data* data, const CodeWarning& log)
{
   Debug::ft("Editor.FixData");

   switch(log.warning_)
   {
   case DataUnused:
      return EraseItem(data);
   case DataInitOnly:
      return EraseItem(data);
   case DataWriteOnly:
      return EraseItem(data);
   case DataCouldBeConst:
      return TagAsConstData(data);
   case DataCouldBeConstPtr:
      return TagAsConstPointer(data);
   case DataShouldBeStatic:
      return TagAsStaticData(data);
   }

   return Report("Internal error: unsupported data warning.");
}

//------------------------------------------------------------------------------

word Editor::FixDatas(const CodeWarning& log)
{
   Debug::ft("Editor.FixDatas");

   if(log.item_->Type() != Cxx::Data)
   {
      return Report("Internal error: warning is not for data.");
   }

   //  Fix the data's declaration and definition (if distinct).
   //
   auto data = static_cast<Data*>(log.item_);
   DataVector datas;
   GetDatas(data, datas);

   for(auto d = datas.cbegin(); d != datas.cend(); ++d)
   {
      auto& editor = (*d)->GetFile()->GetEditor();
      auto rc = editor.FixData(*d, log);
      editor.ReportFixInFile(&log, rc);
   }

   return EditCompleted;
}

//------------------------------------------------------------------------------

word Editor::FixFunction(Function* func, const CodeWarning& log)
{
   Debug::ft("Editor.FixFunction");

   switch(log.warning_)
   {
   case ArgumentUnused:
      return EraseParameter(func, log.offset_);
   case FunctionUnused:
      return EraseItem(func);
   case VirtualAndPublic:
      return SplitVirtualFunction(func);
   case VirtualDefaultArgument:
      return EraseDefaultValue(func, log.offset_);
   case ArgumentCouldBeConstRef:
      return TagAsConstReference(func, log.offset_);
   case ArgumentCouldBeConst:
      return TagAsConstArgument(func, log.offset_);
   case FunctionCouldBeConst:
      return TagAsConstFunction(func);
   case FunctionCouldBeStatic:
      return TagAsStaticClassFunction(func);
   case FunctionCouldBeDefaulted:
      return TagAsDefaulted(func);
   case CouldBeNoexcept:
      return TagAsNoexcept(func);
   case ShouldNotBeNoexcept:
      return EraseNoexceptTag(func);
   case FunctionCouldBeMember:
      return ChangeFunctionToMember(func, log.offset_);
   case FunctionShouldBeStatic:
      return TagAsStaticFreeFunction(func);
   }

   return Report("Internal error: unsupported function warning.");
}

//------------------------------------------------------------------------------

word Editor::FixFunctions(const CodeWarning& log)
{
   Debug::ft("Editor.FixFunctions");

   if(log.item_->Type() != Cxx::Function)
   {
      return Report("Internal error: warning is not for a function.");
   }

   //  Create a list of all the function declarations and definitions that
   //  are associated with the log.
   //
   auto func = static_cast<Function*>(log.item_);
   FunctionVector funcs;
   GetOverrides(func, funcs);

   for(auto f = funcs.cbegin(); f != funcs.cend(); ++f)
   {
      auto& editor = (*f)->GetFile()->GetEditor();
      auto rc = editor.FixFunction(*f, log);
      editor.ReportFixInFile(&log, rc);
   }

   return EditCompleted;
}

//------------------------------------------------------------------------------

word Editor::FixInvoker(const Function* func, const CodeWarning& log)
{
   Debug::ft("Editor.FixInvoker");

   switch(log.warning_)
   {
   case ArgumentUnused:
      return EraseArgument(func, log.offset_);
   case VirtualDefaultArgument:
      return InsertArgument(func, log.offset_);
   case FunctionCouldBeMember:
      return ChangeInvokerToMember(func, log.offset_);
   }

   return Report("Internal error: unexpected invoker warning.");
}

//------------------------------------------------------------------------------

word Editor::FixInvokers(const CodeWarning& log)
{
   Debug::ft("Editor.FixInvokers");

   //  Use FixFunctions to modify all of the function signatures, and then
   //  use the cross-reference to find and modify all of the invocations.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::FixLog(const CodeWarning& log)
{
   Debug::ft("Editor.FixLog");

   //  Only a status_ of NotFixed should come through here.  The others
   //  should have been intercepted by the switch statement in Fix.
   //
   switch(log.status_)
   {
   case NotSupported:
      return Report(NotImplemented);
   case Revoked:
      return Report("This warning was cancelled after it was logged.");
   case Deleted:
      return Report("The code associated with this warning was deleted.");
   case NotFixed:
      break;
   case Fixed:
   case Pending:
      return Report("This warning has already been fixed.");
   }

   return FixWarning(log);
}

//------------------------------------------------------------------------------

word Editor::FixReference(const CxxToken* item, const CodeWarning& log)
{
   Debug::ft("Editor.FixReference");

   switch(log.warning_)
   {
   case DataWriteOnly:
      return EraseAssignment(item);
   }

   return Report("Internal error: unsupported data warning.");
}

//------------------------------------------------------------------------------

word Editor::FixReferences(const CodeWarning& log)
{
   Debug::ft("Editor.FixReferences");

   if(log.item_->Type() != Cxx::Data)
   {
      return Report("Internal error: warning is not for data.");
   }

   //  Fix references to the data, followed by the data itself.  Before
   //  erasing init-only or write-only data, confirm that doing so will
   //  not eliminate side effects that need to be retained.
   //
   auto data = static_cast<Data*>(log.item_);
   auto refs = data->GetNonLocalRefs();

   switch(log.warning_)
   {
   case DataInitOnly:
   case DataWriteOnly:
   {
      DataVector datas;
      GetDatas(data, datas);

      *Cli_->obuf << "The data is used in the following locations." << CRLF;
      *Cli_->obuf << "Erasing it could remove needed side effects." << CRLF;

      Flags options(Code_Mask | NoAC_Mask);

      for(auto d = datas.cbegin(); d != datas.cend(); ++d)
      {
         (*d)->Display(*Cli_->obuf, spaces(IndentSize()), options);
      }

      for(auto r = refs.cbegin(); r != refs.cend(); ++r)
      {
         auto file = (*r)->GetFile();
         auto& editor = file->GetEditor();
         auto code = editor.GetCode((*r)->GetPos());
         *Cli_->obuf << code;
      }

      if(!Cli_->BoolPrompt("Erase all appearances of this data?"))
         return Report("Warning skipped.");
   }
   }

   for(auto r = refs.cbegin(); r != refs.cend(); ++r)
   {
      auto& editor = (*r)->GetFile()->GetEditor();
      auto rc = editor.FixReference(*r, log);
      editor.ReportFixInFile(&log, rc);
   }

   return FixDatas(log);
}

//------------------------------------------------------------------------------

WarningStatus Editor::FixStatus(const CodeWarning& log) const
{
   Debug::ft("Editor.FixStatus");

   //  Some logs may appear multiple times, but all of them get fixed when
   //  the first one gets fixed.
   //
   switch(log.warning_)
   {
   case IncludeNotSorted:
   case IncludeFollowsCode:
      if((log.status_ == NotFixed) && sorted_) return Fixed;
      break;

   case FunctionNotSorted:
      if((log.status_ == NotFixed) && FunctionsWereSorted(log)) return Fixed;
      break;

   case OverrideNotSorted:
      if((log.status_ == NotFixed) && OverridesWereSorted(log)) return Fixed;
      break;
   }

   return log.status_;
}

//------------------------------------------------------------------------------

word Editor::FixWarning(const CodeWarning& log)
{
   Debug::ft("Editor.FixWarning");

   switch(log.warning_)
   {
   case UseOfNull:
      return ReplaceNull(log);
   case PtrTagDetached:
      return AdjustTags(log);
   case RefTagDetached:
      return AdjustTags(log);
   case RedundantSemicolon:
      return EraseSemicolon(log);
   case RedundantConst:
      return EraseConst(log);
   case DefineNotAtFileScope:
      return MoveDefine(log);
   case IncludeFollowsCode:
      return SortIncludes();
   case IncludeGuardMissing:
      return InsertIncludeGuard(log);
   case IncludeNotSorted:
      return SortIncludes();
   case IncludeDuplicated:
      return EraseItem(log.item_);
   case IncludeAdd:
      return InsertInclude(log);
   case IncludeRemove:
      return EraseItem(log.item_);
   case RemoveOverrideTag:
      return EraseOverrideTag(log);
   case UsingInHeader:
      return ReplaceUsing(log);
   case UsingDuplicated:
      return EraseItem(log.item_);
   case UsingAdd:
      return InsertUsing(log);
   case UsingRemove:
      return EraseItem(log.item_);
   case ForwardAdd:
      return InsertForward(log);
   case ForwardRemove:
      return EraseForward(log);
   case ArgumentUnused:
      return FixInvokers(log);
   case ClassUnused:
      return EraseClass(log);
   case DataUnused:
      return FixReferences(log);
   case EnumUnused:
      return EraseItem(log.item_);
   case EnumeratorUnused:
      return EraseItem(log.item_);
   case FriendUnused:
      return EraseItem(log.item_);
   case FunctionUnused:
      return FixFunctions(log);
   case TypedefUnused:
      return EraseItem(log.item_);
   case ForwardUnresolved:
      return EraseForward(log);
   case FriendUnresolved:
      return EraseItem(log.item_);
   case FriendAsForward:
      return InsertForward(log);
   case HidesInheritedName:
      return ReplaceName(log);
   case ClassCouldBeNamespace:
      return ChangeClassToNamespace(log);
   case ClassCouldBeStruct:
      return ChangeClassToStruct(log);
   case StructCouldBeClass:
      return ChangeStructToClass(log);
   case RedundantAccessControl:
      return EraseAccessControl(log);
   case ItemCouldBePrivate:
      return ChangeAccess(log, Cxx::Private);
   case ItemCouldBeProtected:
      return ChangeAccess(log, Cxx::Protected);
   case AnonymousEnum:
      return InsertEnumName(log);
   case DataUninitialized:
      return InsertDataInit(log);
   case DataInitOnly:
      return FixReferences(log);
   case DataWriteOnly:
      return FixReferences(log);
   case DataCouldBeConst:
      return FixDatas(log);
   case DataCouldBeConstPtr:
      return FixDatas(log);
   case DataNeedNotBeMutable:
      return EraseMutableTag(log);
   case ImplicitPODConstructor:
      return InsertPODCtor(log);
   case ImplicitConstructor:
      return InsertSpecialFunctions(log.item_);
   case ImplicitCopyConstructor:
      return InsertSpecialFunctions(log.item_);
   case ImplicitCopyOperator:
      return InsertSpecialFunctions(log.item_);
   case PublicConstructor:
      return ChangeSpecialFunction(log);
   case NonExplicitConstructor:
      return TagAsExplicit(log);
   case MemberInitMissing:
      return InsertMemberInit(log);
   case MemberInitNotSorted:
      return MoveMemberInit(log);
   case ImplicitDestructor:
      return InsertSpecialFunctions(log.item_);
   case VirtualDestructor:
      return ChangeAccess(log, Cxx::Public);
   case NonVirtualDestructor:
      return ChangeSpecialFunction(log);
   case RuleOf3DtorNoCopyCtor:
      return InsertSpecialFunctions(log.item_);
   case RuleOf3DtorNoCopyOper:
      return InsertSpecialFunctions(log.item_);
   case RuleOf3CopyCtorNoOper:
      return InsertSpecialFunctions(log.item_);
   case RuleOf3CopyOperNoCtor:
      return InsertSpecialFunctions(log.item_);
   case FunctionNotDefined:
      return EraseItem(log.item_);
   case PureVirtualNotDefined:
      return InsertPureVirtual(log);
   case VirtualAndPublic:
      return FixFunctions(log);
   case FunctionNotOverridden:
      return EraseVirtualTag(log);
   case RemoveVirtualTag:
      return EraseVirtualTag(log);
   case OverrideTagMissing:
      return TagAsOverride(log);
   case VoidAsArgument:
      return EraseVoidArgument(log);
   case AnonymousArgument:
      return RenameArgument(log);
   case DefinitionRenamesArgument:
      return RenameArgument(log);
   case OverrideRenamesArgument:
      return RenameArgument(log);
   case VirtualDefaultArgument:
      return FixInvokers(log);
   case ArgumentCouldBeConstRef:
      return FixFunctions(log);
   case ArgumentCouldBeConst:
      return FixFunctions(log);
   case FunctionCouldBeConst:
      return FixFunctions(log);
   case FunctionCouldBeStatic:
      return FixFunctions(log);
   case FunctionCouldBeFree:
      return ChangeMemberToFree(log);
   case StaticFunctionViaMember:
      return ChangeOperator(log);
   case UseOfTab:
      return ConvertTabsToBlanks();
   case Indentation:
      return AdjustIndentation(log);
   case TrailingSpace:
      return EraseTrailingBlanks();
   case AdjacentSpaces:
      return EraseAdjacentSpaces(log);
   case AddBlankLine:
      return InsertBlankLine(log);
   case RemoveLine:
      return EraseBlankLine(log);
   case LineLength:
      return InsertLineBreak(log);
   case FunctionNotSorted:
      return SortFunctions(log);
   case HeadingNotStandard:
      return ReplaceHeading(log);
   case IncludeGuardMisnamed:
      return RenameIncludeGuard(log);
   case DebugFtNotInvoked:
      return InsertDebugFtCall(log);
   case DebugFtNameMismatch:
      return RenameDebugFtArgument(log);
   case DebugFtNameDuplicated:
      return RenameDebugFtArgument(log);
   case DisplayNotOverridden:
      return InsertDisplay(log);
   case PatchNotOverridden:
      return InsertPatch(log);
   case FunctionCouldBeDefaulted:
      return FixFunctions(log);
   case InitCouldUseConstructor:
      return ChangeAssignmentToCtorCall(log);
   case CouldBeNoexcept:
      return FixFunctions(log);
   case ShouldNotBeNoexcept:
      return FixFunctions(log);
   case UseOfSlashAsterisk:
      return ReplaceSlashAsterisk(log);
   case RemoveLineBreak:
      return EraseLineBreak(log);
   case CopyCtorConstructsBase:
      return InsertCopyCtorCall(log);
   case FunctionCouldBeMember:
      return FixInvokers(log);
   case ExplicitConstructor:
      return EraseExplicitTag(log);
   case BitwiseOperatorOnBoolean:
      return ChangeOperator(log);
   case DebugFtCanBeLiteral:
      return InlineDebugFtArgument(log);
   case UnnecessaryCast:
      return EraseCast(log);
   case ExcessiveCast:
      return ChangeCast(log);
   case DataCouldBeFree:
      return ChangeMemberToFree(log);
   case ConstructorNotPrivate:
      return ChangeSpecialFunction(log);
   case DestructorNotPrivate:
      return ChangeSpecialFunction(log);
   case RedundantScope:
      return EraseScope(log);
   case OperatorSpacing:
      return AdjustOperator(log);
   case PunctuationSpacing:
      return AdjustPunctuation(log);
   case CopyCtorNotDeleted:
      return DeleteSpecialFunction(log);
   case CopyOperNotDeleted:
      return DeleteSpecialFunction(log);
   case CtorCouldBeDeleted:
      return DeleteSpecialFunction(log);
   case NoJumpOrFallthrough:
      return InsertFallthrough(log);
   case OverrideNotSorted:
      return SortOverrides(log);
   case DataShouldBeStatic:
      return FixDatas(log);
   case FunctionShouldBeStatic:
      return FixFunctions(log);
   case FunctionCouldBeDemoted:
      return DemoteFunction(log);
   case NoEndlineAtEndOfFile:
      return AppendEndline();
   case AutoCopiesReference:
      return ChangeAuto(log);
   case AutoCopiesConstReference:
      return ChangeAuto(log);
   case AutoCopiesObject:
      return ChangeAuto(log);
   case AutoCopiesConstObject:
      return ChangeAuto(log);
   case AutoShouldBeConst:
      return ChangeAuto(log);
   }

   return Report(NotImplemented);
}

//------------------------------------------------------------------------------

word Editor::Format(string& expl)
{
   Debug::ft("Editor.Format");

   AppendEndline();
   EraseTrailingBlanks();
   AdjustVertically();
   auto rc = Write();
   expl = GetExpl();
   return rc;
}

//------------------------------------------------------------------------------

bool Editor::FunctionsWereSorted(const CodeWarning& log) const
{
   Debug::ft("Editor.FunctionsWereSorted");

   auto area = log.item_->GetArea();
   const auto& warnings = file_->GetWarnings();

   for(auto w = warnings.cbegin(); w != warnings.cend(); ++w)
   {
      if(w->warning_ == FunctionNotSorted)
      {
         if((w->status_ >= Pending) && (w->item_->GetArea() == area))
            return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

string Editor::GetDefnCode(const CxxScope* defn, CxxToken*& ftarg) const
{
   Debug::ft("Editor.GetDefnCode");

   size_t begin, left, end;
   defn->GetSpan3(begin, left, end);
   const auto& lexer = defn->GetFile()->GetLexer();
   auto code = lexer.Source().substr(begin, end - begin + 1);

   ftarg = nullptr;
   auto pos = Find(left, "Debug::ft");

   if(pos < end)
   {
      pos = Find(pos, "(");
      pos = FindNonBlank(pos + 1);
      ftarg = file_->PosToItem(pos);
   }

   return code;
}

//------------------------------------------------------------------------------

CxxItemVector Editor::GetItemsForDefn(const CxxScope* defn) const
{
   Debug::ft("Editor.GetItemsForDefn");

   CxxItemVector items;
   if(defn == nullptr) return items;

   const auto& fileItems = file_->Items();

   //  Return the items above DEFN that are referenced by DEFN.
   //
   for(auto i = fileItems.cbegin(); i != fileItems.cend(); ++i)
   {
      if(*i == defn)
      {
         size_t begin, end;
         defn->GetSpan2(begin, end);

         while(i != fileItems.cbegin())
         {
            --i;
            if(!ItemIsUsedBetween(*i, begin, end)) break;
            items.push_back(*i);
         }

         break;
      }
   }

   std::sort(items.begin(), items.end(), IsSortedByPos);
   return items;
}

//------------------------------------------------------------------------------

ItemOffsets Editor::GetOffsets(const CxxToken* item) const
{
   Debug::ft("Editor.GetOffsets");

   ItemOffsets offsets;

   if(item == nullptr) return offsets;

   size_t begin, end;
   if(!item->GetSpan2(begin, end)) return offsets;

   //  See what follows ITEM.
   //
   auto pos = NextBegin(end);
   auto type = PosToType(pos);

   if(type == BlankLine)
   {
      type = PosToType(NextBegin(pos));

      if(type == RuleComment)
         offsets.below_ = OffsetRule;
      else
         offsets.below_ = OffsetBlank;
   }

   //  See what precedes ITEM.
   //
   pos = begin;

   while(true)
   {
      pos = PrevBegin(pos);
      type = PosToType(pos);

      switch(type)
      {
      case BlankLine:
         offsets.above_ = OffsetBlank;
         [[fallthrough]];
      case FunctionName:
         continue;

      case RuleComment:
         offsets.above_ = OffsetRule;
         [[fallthrough]];
      default:
         return offsets;
      }
   }
}

//------------------------------------------------------------------------------

bool Editor::GetSpan(const CxxToken* item, size_t& begin, size_t& end)
{
   Debug::ft("Editor.GetSpan");

   if(item == nullptr)
   {
      begin = string::npos;
      end = string::npos;
      return false;
   }

   if(!item->GetSpan2(begin, end)) return false;

   if(code_[begin] == ',')
   {
      //  The cut begins at a comma, which could be on the previous line.
      //  If a comment follows it, replace it with a space and cut from
      //  the beginning of the next line.
      //
      if(CommentFollows(begin + 1))
      {
         code_[begin] = SPACE;
         begin = NextBegin(begin);
      }
   }
   else
   {
      //  Cut from the start of BEGIN's line unless other code precedes
      //  or follows the item.
      //
      if(IsFirstNonBlank(begin) && NoCodeFollows(end + 1))
      {
         begin = CurrBegin(begin);
      }
   }

   //  When the cuts ends at a right brace, also cut a semicolon that
   //  follows immediately.
   //
   if(code_[end] == '}')
   {
      auto pos = NextPos(end + 1);
      if((pos != string::npos) && (code_[pos] == ';')) end = pos;
   }

   //  Cut any comment or whitespace that follows on the last line.
   //
   if(NoCodeFollows(end + 1))
      end = CurrEnd(end);
   else
      end = LineFindNonBlank(end + 1) - 1;

   //  If entire lines of code that aren't immediately followed by more code
   //  have been selected, include any comment that precedes the code, as it
   //  almost certainly refers only to the code being cut.  But don't do this
   //  for a preprocessor directive or function data, whose preceding comments
   //  usually apply to code that follows, even if a blank line follows ITEM.
   //
   if((begin == CurrBegin(begin)) && (end == CurrEnd(end)))
   {
      if(code_[begin] == '#')
      {
         return true;
      }

      if((item->Type() == Cxx::Data) &&
         (item->GetScope()->Type() == Cxx::Block))
      {
         return true;
      }

      if(!CodeFollowsImmediately(end))
      {
         begin = IntroStart(begin, false);
      }
   }

   return true;
}

//------------------------------------------------------------------------------

size_t Editor::IncludesBegin() const
{
   Debug::ft("Editor.IncludesBegin");

   for(size_t pos = 0; pos != string::npos; pos = NextBegin(pos))
   {
      if(CompareCode(pos, HASH_INCLUDE_STR) == 0) return pos;
   }

   return string::npos;
}

//------------------------------------------------------------------------------

word Editor::Indent(size_t pos)
{
   Debug::ft("Editor.Indent");

   auto info = GetLineInfo(pos);
   if(info == nullptr) return NotFound("Position for indentation");
   auto begin = CurrBegin(pos);
   auto first = LineFindFirst(pos);
   auto curr = first - begin;
   auto depth = info->depth;
   if(info->continuation && LineTypeAttr::Attrs[info->type].isCode) ++depth;
   auto indent = depth * IndentSize();

   if(indent > curr)
      Insert(pos, spaces(indent - curr));
   else
      Erase(pos, curr - indent);
   return Changed(pos);
}

//------------------------------------------------------------------------------

word Editor::InlineDebugFtArgument(const CodeWarning& log)
{
   Debug::ft("Editor.InlineDebugFtArgument");

   //  LOG's position is where the Debug::ft call occurs, but log.item_ is
   //  the function in which the Debug::ft call occurs.  Find the fn_name
   //  data item and in-line its string literal in the Debug::ft call.
   //
   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound("Position of Debug::ft");
   auto lpar = code_.find('(', begin);
   if(lpar == string::npos) return NotFound("Debug::ft left parenthesis");
   auto npos = FindNonBlank(lpar + 1);
   if(npos == string::npos) return NotFound("Start of Debug::ft argument");
   auto arg = file_->PosToItem(npos);
   if(arg == nullptr) return NotFound("Debug::ft argument");
   auto aref = arg->Referent();
   if(aref == nullptr) return NotFound("Debug::ft fn_name");
   if(aref->Type() != Cxx::Data) return NotFound("Debug::ft fn_name");

   string fname;
   auto data = static_cast<Data*>(aref);
   if(!data->GetStrValue(fname)) return NotFound("fn_name definition");

   auto literal = QUOTE + fname + QUOTE;
   auto rpar = code_.find(')', lpar);
   if(rpar == string::npos) return NotFound("Debug::ft right parenthesis");
   Replace(lpar + 1, rpar - lpar - 1, literal);
   ReplaceImpl(static_cast<Function*>(log.item_));

   //  Erase the fn_name.  This was deferred until now so as not to invalidate
   //  LPAR, which is an infuriatingly common type of bug when editing code.
   //
   if(EraseItem(data) == EditFailed) return EditFailed;
   return Changed(lpar);
}

//------------------------------------------------------------------------------

size_t Editor::Insert(size_t pos, const string& code)
{
   Debug::ft("Editor.Insert");

   code_.insert(pos, code);
   UpdatePos(Inserted, pos, code.size());
   return pos;
}

//------------------------------------------------------------------------------

void Editor::InsertAboveItemDecl
   (const ItemDeclAttrs& attrs, const string& comment)
{
   Debug::ft("Editor.InsertAboveItemDecl");

   if(attrs.comment_)
   {
      InsertLine(attrs.pos_, strComment(EMPTY_STR, attrs.indent_));
      Insert(attrs.pos_, strComment(comment, attrs.indent_));
   }

   if(attrs.thisctrl_)
   {
      std::ostringstream stream;
      if(attrs.indent_ > 0) stream << spaces(attrs.indent_ - IndentSize());
      stream << attrs.access_ << ':';
      InsertLine(attrs.pos_, stream.str());
   }
   else if(attrs.blank_ == BlankAbove)
   {
      InsertLine(attrs.pos_, EMPTY_STR);
   }
}

//------------------------------------------------------------------------------

void Editor::InsertAboveItemDefn(const ItemDefnAttrs& attrs)
{
   Debug::ft("Editor.InsertAboveItemDefn");

   switch(attrs.offsets_.above_)
   {
   case OffsetRule:
      InsertLine(attrs.pos_, EMPTY_STR);
      InsertLine(attrs.pos_, SingleRule());
      [[fallthrough]];
   case OffsetBlank:
      InsertLine(attrs.pos_, EMPTY_STR);
   }
}

//------------------------------------------------------------------------------

word Editor::InsertArgument(const Function* func, word offset)
{
   Debug::ft("Editor.InsertArgument");

   //  Change all invocations of FUNC so that any which use the default
   //  value for this argument pass the default value explicitly.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

void Editor::InsertBelowItemDecl(const ItemDeclAttrs& attrs)
{
   Debug::ft("Editor.InsertBelowItemDecl");

   if(attrs.nextctrl_ != Cxx::Access_N)
   {
      std::ostringstream access;
      access << attrs.nextctrl_ << ':';
      InsertLine(attrs.pos_, access.str());
   }
   else if(attrs.blank_ == BlankBelow)
   {
      InsertLine(attrs.pos_, EMPTY_STR);
   }
}

//------------------------------------------------------------------------------

void Editor::InsertBelowItemDefn(const ItemDefnAttrs& attrs)
{
   Debug::ft("Editor.InsertBelowItemDefn");

   switch(attrs.offsets_.below_)
   {
   case OffsetRule:
      InsertLine(attrs.pos_, EMPTY_STR);
      InsertLine(attrs.pos_, SingleRule());
      [[fallthrough]];
   case OffsetBlank:
      InsertLine(attrs.pos_, EMPTY_STR);
   }
}

//------------------------------------------------------------------------------

word Editor::InsertBlankLine(const CodeWarning& log)
{
   Debug::ft("Editor.InsertBlankLine");

   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound("Position for blank line");
   InsertLine(begin, EMPTY_STR);
   return EditSucceeded;
}

//------------------------------------------------------------------------------

word Editor::InsertCopyCtorCall(const CodeWarning& log)
{
   Debug::ft("Editor.InsertCopyCtorCall");

   //  Have this copy constructor invoke its base class copy constructor.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::InsertDataInit(const CodeWarning& log)
{
   Debug::ft("Editor.InsertDataInit");

   //  Initialize this data item to its default value.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::InsertDebugFtCall(const CodeWarning& log)
{
   Debug::ft("Editor.InsertDebugFtCall");

   size_t begin, left, right;
   auto func = static_cast<Function*>(log.item_);
   func->GetSpan3(begin, left, right);
   if(left == string::npos) return NotFound("Function definition");

   //  Get the start of the name for an fn_name declaration and the inline
   //  string literal for the Debug::ft invocation.  Set EXTRA if anything
   //  follows the left brace on the same line.
   //
   string flit, fvar;
   DebugFtNames(func, flit, fvar);
   auto extra = (LineFindNext(left + 1) != string::npos);

   //  There are two possibilities:
   //  o An fn_name is already defined (e.g. for invoking Debug::SwLog), in
   //    which case it will be located before the end of the function.  Its
   //    name will start with FVAR but could be longer if the function is
   //    overloaded, so extract everything to the closing ')' in the call.
   //  o An fn_name is not already defined, in which case the string literal
   //    (FLIT) can be used.
   //
   string arg;

   for(auto pos = left; (pos < right) && arg.empty(); pos = NextBegin(pos))
   {
      auto start = LineFind(pos, fvar);

      if(start != string::npos)
      {
         auto end = code_.find_first_not_of(ValidNextChars, start);
         if(end == string::npos) return NotFound("End of fn_name");
         arg = code_.substr(start, end - start);
      }
   }

   if(arg.empty())
   {
      //  No fn_name was defined, so use FLIT.  If another function already
      //  uses that name, prompt the user for a suffix.
      //
      if(!EnsureUniqueDebugFtName(func, flit, arg))
         return Report(FixSkipped);
   }

   //  Create the call to Debug::ft at the top of the function, after its
   //  left brace:
   //  o If something followed the left brace, push it down.
   //  o Insert a blank line after the call to Debug::ft.
   //  o Insert the call to Debug::ft.
   //  o If the left brace wasn't at the start of its line, push it down.
   //
   if(extra) InsertLineBreak(left + 1);
   auto below = NextBegin(left);
   InsertLine(below, EMPTY_STR);
   auto call = DebugFtCode(arg);
   InsertLine(below, call);
   if(!IsFirstNonBlank(left)) InsertLineBreak(left);
   ReplaceImpl(func);
   return Changed(below);
}

//------------------------------------------------------------------------------

word Editor::InsertDisplay(const CodeWarning& log)
{
   Debug::ft("Editor.InsertDisplay");

   //  Declare an override and put "To be implemented" in the definition, with
   //  an invocation of the base class's Display function.  A more ambitious
   //  implementation would display members or invoke *their* Display functions.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::InsertEnumName(const CodeWarning& log)
{
   Debug::ft("Editor.InsertEnumName");

   //  Prompt for the enum's name and insert it.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::InsertFallthrough(const CodeWarning& log)
{
   Debug::ft("Editor.InsertFallthrough");

   //  Add a [[fallthrough]]; at the end of this case label.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::InsertForward(const CodeWarning& log)
{
   Debug::ft("Editor.InsertForward(log)");

   //  log.info_ provides the forward's namespace and any template parameters.
   //
   string forward = spaces(IndentSize()) + log.info_ + ';';
   auto srPos = forward.find(SCOPE_STR);
   if(srPos == string::npos) return NotFound("Forward's namespace.");

   //  Extract the namespace; rfind avoids the initial "class" in a forward
   //  declaration for template<class T> class ClassTemplateName.
   //
   auto areaPos = forward.rfind("class ");
   if(areaPos == string::npos) areaPos = forward.rfind("struct ");
   if(areaPos == string::npos) areaPos = forward.rfind("union ");
   if(areaPos == string::npos) return NotFound("Forward's area type");

   //  Set NSPACE to "namespace <ns>", where <ns> is the symbol's namespace.
   //  Erase <ns> from FORWARD and then decide where to insert the forward
   //  declaration.
   //
   string nspace = NAMESPACE_STR;
   auto nsPos = forward.find(SPACE, areaPos);
   auto nsName = forward.substr(nsPos, srPos - nsPos);
   nspace += nsName;
   forward.erase(nsPos + 1, srPos - nsPos + 1);
   auto begin = CodeBegin();

   for(auto pos = PrologEnd(); pos != string::npos; pos = NextBegin(pos))
   {
      if(CompareCode(pos, NAMESPACE_STR) == 0)
      {
         //  If this namespace matches NSPACE, add the declaration to it.
         //
         if(CompareCode(pos, nspace) == 0)
         {
            return InsertForward(pos, nsName.substr(1), forward);
         }
      }
      else if((CompareCode(pos, USING_STR) == 0) || (pos == begin))
      {
         //  We have now passed any existing forward declarations, so add
         //  the new declaration here, along with its namespace.
         //
         return InsertNamespaceForward(pos, nspace, forward);
      }
   }

   return Report("Failed to insert forward declaration.");
}

//------------------------------------------------------------------------------

word Editor::InsertForward(size_t pos,
   const string& nspace, const string& forward)
{
   Debug::ft("Editor.InsertForward(pos)");

   //  POS references a namespace that matches the one for a new forward
   //  declaration.  Insert the new declaration alphabetically within the
   //  declarations that already appear in this namespace.  Note that it
   //  may already have been inserted while fixing another warning.
   //
   for(pos = NextBegin(pos); pos != string::npos; pos = NextBegin(pos))
   {
      auto first = LineFindFirst(pos);
      if(code_[first] == '{') continue;

      auto comp = CompareCode(pos, forward);
      if(comp == 0) return Report("Previously inserted.");

      if((comp > 0) || (code_[first] == '}'))
      {
         InsertLine(pos, forward);
         ParseFileItem(pos, FindNamespace(nspace));
         return Changed(pos);
      }
   }

   return Report("Failed to insert forward declaration.");
}

//------------------------------------------------------------------------------

word Editor::InsertInclude(const CodeWarning& log)
{
   Debug::ft("Editor.InsertInclude");

   //  The file name for the new #include directive appears in log.info_ and
   //  is enclosed in quotes or angle brackets.
   //
   auto filename = log.info_.substr(1, log.info_.size() - 2);
   IncludePtr incl(new Include(filename, log.info_.front() == '<'));
   size_t pos = string::npos;
   incl->SetLoc(file_, pos, false);
   incl->CalcGroup();

   const auto& incls = file_->Includes();

   for(auto next = incls.cbegin(); next != incls.cend(); ++next)
   {
      if(IncludesAreSorted(incl, *next))
      {
         pos = (*next)->GetPos();
         break;
      }
   }

   if(pos == string::npos)
   {
      //  If we get here, the new #include must be added after all the others.
      //  The last #include should be followed by a blank line, so insert one
      //  if necessary.
      //
      if(!incls.empty())
         pos = NextBegin(incls.back()->GetPos());
      else
         pos = PrologEnd();

      if(!IsBlankLine(pos))
      {
         InsertLine(pos, EMPTY_STR);
      }
   }

   auto code = string(HASH_INCLUDE_STR) + SPACE + log.info_;
   InsertLine(pos, code);
   file_->InsertInclude(incl, pos);
   return Changed(pos);
}

//------------------------------------------------------------------------------

word Editor::InsertIncludeGuard(const CodeWarning& log)
{
   Debug::ft("Editor.InsertIncludeGuard");

   //  Insert the guard before the first #include.  If there isn't one,
   //  insert it before the first line of code.  If there isn't any code,
   //  insert it at the end of the file.
   //
   auto pos = IncludesBegin();
   if(pos == string::npos) pos = PrologEnd();
   string guardName = log.File()->MakeGuardName();
   string code = "#define " + guardName;
   InsertLine(pos, EMPTY_STR);
   InsertLine(pos, code);
   code = "#ifndef " + guardName;
   auto ifnPos = InsertLine(pos, code);
   auto defPos = code_.find(HASH_DEFINE_STR, ifnPos);
   code = string(HASH_ENDIF_STR) + CRLF_STR;
   auto endPos = Insert(code_.size(), code);

   //  The same Parser instance is used because when parsing an #endif, the
   //  parser must have noted an unresolved #if/#ifdef/#ifndef.
   //
   auto ns = Singleton<CxxRoot>::Instance()->GlobalNamespace();
   ParserPtr parser(new Parser());
   parser->ParseFileItem(code_, ifnPos, file_, ns);
   parser->ParseFileItem(code_, defPos, nullptr, ns);
   parser->ParseFileItem(code_, endPos, nullptr, ns);
   return Changed(pos);
}

//------------------------------------------------------------------------------

size_t Editor::InsertLine(size_t pos, const string& code)
{
   Debug::ft("Editor.InsertLine");

   if(pos >= code_.size()) return string::npos;
   auto copy = code;
   if(copy.empty() || (copy.back() != CRLF)) copy.push_back(CRLF);
   return Insert(pos, copy);
}

//------------------------------------------------------------------------------

word Editor::InsertLineBreak(size_t pos)
{
   Debug::ft("Editor.InsertLineBreak(pos)");

   auto begin = CurrBegin(pos);
   if(begin == string::npos) return NotFound("Line break insertion position");
   auto end = CurrEnd(pos);
   if((pos == begin) || (pos == end)) return
      Report("Error: inserting line break at beginning or end of line.");
   if(IsBlankLine(pos)) return
      Report("Error: inserting line break in an empty line");
   Insert(pos, CRLF_STR);
   return Indent(NextBegin(pos));
}

//------------------------------------------------------------------------------

word Editor::InsertLineBreak(const CodeWarning& log)
{
   Debug::ft("Editor.InsertLineBreak(log)");

   //  Consider parentheses, lexical level, binary operators...
   //
   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound("Position for line break");

   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::InsertMemberInit(const CodeWarning& log)
{
   Debug::ft("Editor.InsertMemberInit");

   //  Initialize the member to its default value.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::InsertNamespaceForward
   (size_t pos, const string& nspace, const string& forward)
{
   Debug::ft("Editor.InsertNamespaceForward");

   //  Insert a new forward declaration, along with an enclosing namespace,
   //  at POS.  Offset it with blank lines.
   //
   InsertLine(pos, EMPTY_STR);
   InsertLine(pos, "}");
   InsertLine(pos, forward);
   InsertLine(pos, "{");
   auto npos = InsertLine(pos, nspace);
   InsertLine(pos, EMPTY_STR);
   pos = Find(pos, forward);
   ParseFileItem(npos, nullptr);
   return Changed(pos);
}

//------------------------------------------------------------------------------

fixed_string PatchComment = "Overridden for patching.";
fixed_string PatchReturn = "void";
fixed_string PatchSignature = "Patch(sel_t selector, void* arguments)";
fixed_string PatchInvocation = "Patch(selector, arguments)";

word Editor::InsertPatch(const CodeWarning& log)
{
   Debug::ft("Editor.InsertPatch");

   //  Extract the name of the function to insert.  (It will be "Patch",
   //  but this code might eventually be generalized for other functions.)
   //
   auto cls = static_cast<Class*>(log.item_);
   auto name = log.GetNewFuncName();
   if(name.empty()) return Report("Log did not specify function name.");

   //  Find out where the function's declaration should be inserted and which
   //  file should contain its definition.  Then insert the declaration.
   //
   ItemDeclAttrs declAttrs(Cxx::Function, Cxx::Public);
   declAttrs.over_ = true;
   auto rc = FindItemDeclLoc(cls, name, declAttrs);
   if(rc != EditContinue) return Report(UnspecifiedFailure, rc);
   auto file = FindFuncDefnFile(cls, name);
   if(file == nullptr) return NotFound("File for definition");
   InsertPatchDecl(cls, declAttrs);

   //  The definition is usually in another file, so access that file's
   //  editor, find out where the function should be defined in that file,
   //  and insert the definition there.
   //
   auto& editor = file->GetEditor();
   ItemDefnAttrs defnAttrs(FuncOther);
   rc = editor.FindFuncDefnLoc(file, cls, name, defnAttrs);
   if(rc != EditContinue) return rc;
   editor.InsertPatchDefn(cls, defnAttrs);
   auto pos = Find(defnAttrs.pos_, PatchSignature);
   return Changed(pos);
}

//------------------------------------------------------------------------------

void Editor::InsertPatchDecl(Class* cls, const ItemDeclAttrs& attrs)
{
   Debug::ft("Editor.InsertPatchDecl");

   InsertBelowItemDecl(attrs);

   string code = PatchReturn;
   code.push_back(SPACE);
   code.append(PatchSignature);
   code.push_back(SPACE);
   code.append(OVERRIDE_STR);
   code.push_back(';');
   InsertLine(attrs.pos_, strCode(code, 1));
   ParseClassItem(attrs.pos_, cls, attrs.access_);

   InsertAboveItemDecl(attrs, PatchComment);
}

//------------------------------------------------------------------------------

void Editor::InsertPatchDefn(const Class* cls, const ItemDefnAttrs& attrs)
{
   Debug::ft("Editor.InsertPatchDefn");

   InsertBelowItemDefn(attrs);

   InsertLine(attrs.pos_, "}");

   auto base = cls->BaseClass();
   auto code = base->Name();
   code.append(SCOPE_STR);
   code.append(PatchInvocation);
   code.push_back(';');
   InsertLine(attrs.pos_, strCode(code, 1));

   InsertLine(attrs.pos_, "{");

   code = PatchReturn;
   code.push_back(SPACE);
   code.append(cls->Name());
   code.append(SCOPE_STR);
   code.append(PatchSignature);
   InsertLine(attrs.pos_, code);
   ParseFileItem(attrs.pos_, cls->GetSpace());

   InsertAboveItemDefn(attrs);
}

//------------------------------------------------------------------------------

word Editor::InsertPODCtor(const CodeWarning& log)
{
   Debug::ft("Editor.InsertPODCtor");

   //  Declare and define a constructor that initializes POD members to
   //  their default values.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

size_t Editor::InsertPrefix(size_t pos, const string& prefix)
{
   Debug::ft("Editor.InsertPrefix");

   auto first = LineFindFirst(pos);
   if(first == string::npos) return string::npos;

   if(pos + prefix.size() <= first)
      Replace(pos, prefix.size(), prefix);
   else
      Insert(pos, prefix);

   Changed();
   return pos;
}

//------------------------------------------------------------------------------

word Editor::InsertPureVirtual(const CodeWarning& log)
{
   Debug::ft("Editor.InsertPureVirtual");

   //  Insert a definition that invokes Debug::SwLog with strOver(this).
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::InsertSpecialFuncDecl(Class* cls, FunctionRole role)
{
   Debug::ft("Editor.InsertSpecialFuncDecl");

   ItemDeclAttrs declAttrs(Cxx::Function, Cxx::Public, role);
   auto rc = FindSpecialFuncDeclLoc(cls, declAttrs);
   if(rc != EditContinue) return rc;

   auto code = spaces(declAttrs.indent_);
   const auto& className = cls->Name();

   string defn;

   if(!declAttrs.shell_)
   {
      defn += " = ";
      defn += (declAttrs.deleted_ ? DELETE_STR : DEFAULT_STR);
   }

   switch(declAttrs.role_)
   {
   case PureCtor:
      code += className + "()";
      break;

   case CopyCtor:
      code += className;
      code += "(const " + className + "& that)";
      break;

   case CopyOper:
      code += className + "& " + "operator=";
      code += "(const " + className + "& that)";
      break;

   case PureDtor:
      if(declAttrs.virt_) code += string(VIRTUAL_STR) + SPACE;
      code += '~' + className + "()";
      break;

   default:
      return Report("Unexpected special member function.");
   }

   code += defn;
   code.push_back(';');
   InsertBelowItemDecl(declAttrs);
   InsertLine(declAttrs.pos_, code);
   ParseClassItem(declAttrs.pos_, cls, declAttrs.access_);
   code.pop_back();

   auto comment = CreateSpecialFunctionComment(cls, declAttrs);
   InsertAboveItemDecl(declAttrs, comment);

   if(declAttrs.shell_)
   {
      auto file = FindFuncDefnFile(cls, code);
      if(file == nullptr) return NotFound("File for function definition");

      auto& editor = file->GetEditor();

      ItemDefnAttrs defnAttrs(declAttrs.role_);
      rc = editor.FindFuncDefnLoc(file, cls, code, defnAttrs);
      if(rc != EditContinue) return rc;

      //  Insert the function's declaration and definition.
      //
      editor.InsertSpecialFuncDefn(cls, defnAttrs);
   }

   auto pos = code_.find(code, declAttrs.pos_);
   return Changed(pos);
}

//------------------------------------------------------------------------------

void Editor::InsertSpecialFuncDefn(const Class* cls, const ItemDefnAttrs& attrs)
{
   Debug::ft("Editor.InsertSpecialFuncDefn");

   InsertBelowItemDefn(attrs);
   InsertLine(attrs.pos_, "}");

   auto code = spaces(IndentSize());
   code += COMMENT_STR;
   code += "* To be implemented.";
   InsertLine(attrs.pos_, code);
   InsertLine(attrs.pos_, "{");

   const auto& className = cls->Name();
   code.clear();

   if(attrs.role_ == CopyOper)
   {
      code = className;
      code += "& ";
   }

   code += className;
   code += SCOPE_STR;

   switch(attrs.role_)
   {
   case PureCtor:
      code += className + "()";
      break;

   case CopyCtor:
      code += className;
      code += "(const " + className + "& that)";
      break;

   case CopyOper:
      code += className + "& " + "operator=";
      code += "(const " + className + "& that)";
      break;

   case PureDtor:
      code += '~' + className + "()";
      break;
   }

   InsertLine(attrs.pos_, code);
   ParseFileItem(attrs.pos_, cls->GetSpace());
   InsertAboveItemDefn(attrs);
}

//------------------------------------------------------------------------------

constexpr size_t MaxRoleWarning = 15;

struct RoleWarning
{
   const FunctionRole role;  // type of special member function to add
   const Warning log;        // type of log that calls for its addition
};

//  Logs associated with adding a special member function.
//
const RoleWarning SpecialMemberFunctionLogs[MaxRoleWarning] =
{
   { PureCtor, ImplicitConstructor },
   { PureCtor, PublicConstructor },
   { PureCtor, ConstructorNotPrivate },
   { PureCtor, CtorCouldBeDeleted },
   { PureDtor, ImplicitDestructor },
   { PureDtor, NonVirtualDestructor },
   { PureDtor, DestructorNotPrivate },
   { CopyCtor, ImplicitCopyConstructor },
   { CopyCtor, RuleOf3DtorNoCopyCtor },
   { CopyCtor, RuleOf3CopyOperNoCtor },
   { CopyCtor, CopyCtorNotDeleted },
   { CopyOper, ImplicitCopyOperator },
   { CopyOper, RuleOf3DtorNoCopyOper },
   { CopyOper, RuleOf3CopyCtorNoOper },
   { CopyOper, CopyOperNotDeleted }
};

//  Maps a warning to the type of function associated with it.
//
static FunctionRole WarningToRole(Warning log)
{
   for(size_t i = 0; i < MaxRoleWarning; ++i)
   {
      if(SpecialMemberFunctionLogs[i].log == log)
         return SpecialMemberFunctionLogs[i].role;
   }

   return FuncRole_N;
}

//------------------------------------------------------------------------------

word Editor::InsertSpecialFunctions(CxxToken* item)
{
   Debug::ft("Editor.InsertSpecialFunctions");

   //  This coordinates adding all missing special member functions
   //  that can be defined as defaulted or deleted.
   //
   auto cls = static_cast<Class*>(item);
   std::vector<const CodeWarning*> logs;
   std::set<FunctionRole> roles;

   for(size_t i = 0; i < MaxRoleWarning; ++i)
   {
      auto log = CodeWarning::FindWarning
         (file_, SpecialMemberFunctionLogs[i].log, item, 0);

      if((log != nullptr) && (log->status_ == NotFixed))
      {
         logs.push_back(log);
         roles.insert(SpecialMemberFunctionLogs[i].role);
      }
   }

   //  Handle the Rule of 3: if adding the destructor, copy constructor,
   //  or copy operator, add any of the others that are missing.  Don't,
   //  however, try to add a function that is deleted in a base class,
   //  and don't try to add a copy constructor or copy operator to a
   //  class derived from a base class for singletons.
   //
   if((roles.find(PureDtor) != roles.cend()) ||
      (roles.find(CopyCtor) != roles.cend()) ||
      (roles.find(CopyOper) != roles.cend()))
   {
      auto dtor = cls->FindFuncByRole(PureDtor, false);

      if(dtor == nullptr) roles.insert(PureDtor);

      if(!cls->HasSingletonBase())
      {
         auto copy = cls->FindFuncByRole(CopyCtor, true);
         auto oper = cls->FindFuncByRole(CopyOper, true);

         if(copy == nullptr)
            roles.insert(CopyCtor);
         else if((copy->GetClass() != cls) && !copy->IsDeleted())
            roles.insert(CopyCtor);

         if(oper == nullptr)
            roles.insert(CopyOper);
         else if((oper->GetClass() != cls) && !oper->IsDeleted())
            roles.insert(CopyOper);
      }
   }

   if(roles.size() > 1)
   {
      Inform("This will also add related functions...");
   }

   //  Add each function in ROLES and update the warnings associated with it.
   //
   for(int r = PureCtor; r < FuncOther; ++r)
   {
      auto role = static_cast<FunctionRole>(r);

      if(roles.find(role) != roles.cend())
      {
         roles.erase(role);
         auto rc = InsertSpecialFuncDecl(cls, role);
         ReportFix(nullptr, rc);

         if(rc == EditSucceeded)
         {
            for(size_t i = 0; i < logs.size(); ++i)
            {
               if(WarningToRole(logs[i]->warning_) == role)
                  logs[i]->status_ = Pending;
            }
         }
      }
   }

   return EditCompleted;
}

//------------------------------------------------------------------------------

word Editor::InsertUsing(const CodeWarning& log)
{
   Debug::ft("Editor.InsertUsing");

   //  Create the new using statement and decide where to insert it.
   //
   string statement = USING_STR;
   statement.push_back(SPACE);
   statement += log.info_;
   statement.push_back(';');

   auto stop = CodeBegin();
   auto usings = false;

   for(auto pos = PrologEnd(); pos != string::npos; pos = NextBegin(pos))
   {
      if(CompareCode(pos, USING_STR) == 0)
      {
         //  If this using statement is alphabetically after STATEMENT,
         //  add the new statement before it.
         //
         usings = true;

         if(CompareCode(pos, statement) > 0)
         {
            InsertLine(pos, statement);
            ParseFileItem(pos, nullptr);
            return Changed(pos);
         }
      }
      else if((usings && IsBlankLine(pos)) || (pos >= stop))
      {
         //  We have now passed any existing using statements, so add
         //  the new statement here.  If it is the first one, set it
         //  off with blank lines.
         //
         if(!usings) InsertLine(pos, EMPTY_STR);
         InsertLine(pos, statement);
         if(!usings) InsertLine(pos, EMPTY_STR);
         ParseFileItem(pos + 1, nullptr);
         return Changed(pos + 1);
      }
   }

   return Report("Failed to insert using statement.");
}

//------------------------------------------------------------------------------

size_t Editor::LineAfterItem(const CxxToken* item) const
{
   Debug::ft("Editor.LineAfterItem");

   size_t begin, end;
   if(!item->GetSpan2(begin, end)) return string::npos;
   return NextBegin(end);
}

//------------------------------------------------------------------------------

word Editor::MoveDefine(const CodeWarning& log)
{
   Debug::ft("Editor.MoveDefine");

   //  Move this #define directly after the #include directives.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

void Editor::MoveFuncDefn(const FunctionVector& sorted, const Function* func)
{
   Debug::ft("Editor.MoveFuncDefn");

   Function* prev = nullptr;
   Function* next = nullptr;

   for(auto f = sorted.cbegin(); f != sorted.cend(); ++f)
   {
      if(FuncDefnsAreSorted(func, *f))
      {
         next = *f;
         break;
      }

      prev = *f;
   }

   //  Move FUNC so that it follows PREV and precedes NEXT.  Also move any
   //  items defined directly above FUNC and used by FUNC, which will cause
   //  compile errors (forward references) if not moved.
   //
   auto items = GetItemsForDefn(func);

   string code;
   ItemDefnAttrs defnAttrs(func);
   UpdateItemDefnLoc(prev, func, next, defnAttrs);
   InsertBelowItemDefn(defnAttrs);
   auto from = CutCode(func, code);
   if(defnAttrs.pos_ > from) defnAttrs.pos_ -= code.size();
   auto dest = Paste(defnAttrs.pos_, code, from);
   InsertAboveItemDefn(defnAttrs);

   while(!items.empty())
   {
      auto item = items.back();
      items.pop_back();
      MoveItem(item, dest, prev);
   }
}

//------------------------------------------------------------------------------

void Editor::MoveItem(const CxxToken* item, size_t& dest, const CxxScoped* prev)
{
   Debug::ft("Editor.MoveItem");

   //  Cut ITEM and find PREV's span.
   //
   string code;
   auto from = CutCode(item, code);

   if(from < dest)
   {
      dest -= code.size();
   }

   size_t begin, end;
   GetSpan(prev, begin, end);

   //  Add a blank line before and after ITEM if it is commented or if it is
   //  a non-trivial inline.  If PREV is commented, ensure that a blank line
   //  follows it.  If a comment begins at POS, ensure that a blank line
   //  precedes it.
   //
   auto blanks = (IsNonTrivialInline(item, code) ||
      (code.find_first_not_of(WhitespaceChars) == code.find('/')));

   if(blanks || CommentFollows(dest))
   {
      code.push_back(CRLF);
   }

   Paste(dest, code, from);

   if(blanks || CommentFollows(begin) || IsInItemGroup(prev))
   {
      InsertLine(dest, EMPTY_STR);
   }

   Changed();
}

//------------------------------------------------------------------------------

word Editor::MoveMemberInit(const CodeWarning& log)
{
   Debug::ft("Editor.MoveMemberInit");

   //  Move the member to the correct location in the initialization list.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

bool Editor::OverridesWereSorted(const CodeWarning& log) const
{
   Debug::ft("Editor.OverridesWereSorted");

   auto cls = log.item_->GetClass();
   const auto& warnings = file_->GetWarnings();

   for(auto w = warnings.cbegin(); w != warnings.cend(); ++w)
   {
      if(w->warning_ == OverrideNotSorted)
      {
         if((w->status_ >= Pending) && (w->item_->GetClass() == cls))
            return true;
      }
   }

   return false;
}

//------------------------------------------------------------------------------

CxxToken* Editor::ParseClassItem
   (size_t pos, Class* cls, Cxx::Access access) const
{
   Debug::ft("Editor.ParseClassItem");

   ParserPtr parser(new Parser());
   if(!parser->ParseClassItem(code_, pos, cls, access)) return ParseFailed(pos);
   auto item = cls->NewestItem();
   item->UpdateXref(true);
   return item;
}

//------------------------------------------------------------------------------

CxxToken* Editor::ParseFailed(size_t pos) const
{
   Debug::ft("Editor.ParseFailed");

   *Cli_->obuf << spaces(2) << "Incremental parsing failed:" << CRLF;
   *Cli_->obuf << GetCode(pos);
   return nullptr;
}

//------------------------------------------------------------------------------

CxxToken* Editor::ParseFileItem(size_t pos, Namespace* ns) const
{
   Debug::ft("Editor.ParseFileItem");

   if(ns == nullptr) ns = Singleton<CxxRoot>::Instance()->GlobalNamespace();
   ParserPtr parser(new Parser());
   if(!parser->ParseFileItem(code_, pos, file_, ns)) return ParseFailed(pos);
   auto item = file_->NewestItem();
   item->UpdateXref(true);
   return item;
}

//------------------------------------------------------------------------------

fn_name Editor_Paste = "Editor.Paste";

size_t Editor::Paste(size_t pos, const std::string& code, size_t from)
{
   Debug::ft(Editor_Paste);

   if(from != lastErasePos_)
   {
      string expl("Illegal Paste operation: ");
      auto crlf = code.find(CRLF) >= (code.size() - 1);
      if(crlf) expl.push_back(CRLF);
      expl.append(code);
      if(crlf && (code.back() != CRLF)) expl.push_back(CRLF);
      Debug::SwLog(Editor_Paste, expl, from);
      return string::npos;
   }

   code_.insert(pos, code);
   UpdatePos(Pasted, pos, code.size(), from);
   return pos;
}

//------------------------------------------------------------------------------

size_t Editor::PrologEnd() const
{
   Debug::ft("Editor.PrologEnd");

   for(size_t pos = 0; pos != string::npos; pos = NextBegin(pos))
   {
      if(LineTypeAttr::Attrs[PosToType(pos)].isCode) return pos;
   }

   return string::npos;
}

//------------------------------------------------------------------------------

void Editor::QualifyClassItems(CxxScope* decl, string& code) const
{
   Debug::ft("Editor.QualifyClassItems(decl)");

   auto usages = GetItemUsages(decl, true);
   auto cls = decl->GetClass();

   CodeTools::QualifyClassItems(cls, usages.directs, code);
   CodeTools::QualifyClassItems(cls, usages.indirects, code);
}

//------------------------------------------------------------------------------

bool Editor::QualifyOverload(QualName* qname)
{
   Debug::ft("Editor.QualifyOverload");

   //  Find the item in which QNAME appears.
   //
   CxxToken* item = nullptr;
   auto pos = qname->GetPos();
   const auto& items = qname->GetFile()->Items();

   for(auto i = items.cbegin(); i != items.cend(); ++i)
   {
      if((*i)->IsInternal()) continue;

      auto type = (*i)->Type();
      if((type == Cxx::SpaceDefn) || (type == Cxx::Namespace)) continue;

      size_t begin, end;
      (*i)->GetSpan2(begin, end);
      if((pos >= begin) && (pos <= end))
      {
         item = *i;
         break;
      }
   }

   if(item == nullptr) return false;

   auto cls = item->GetClass();
   if(cls == nullptr) return false;

   const auto& name = qname->Name();
   auto funcs = cls->Funcs();
   auto found = false;

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      if((*f)->Name() == name)
      {
         found = true;
         break;
      }
   }

   if(!found) return false;

   //  The class has a function with the same name as the one that is being
   //  made static, so qualify QNAME with its namespace.
   //
   auto space = cls->GetSpace();
   const auto& sname = space->Name();
   auto code = sname + SCOPE_STR;
   qname->AddPrefix(sname, space);
   Insert(pos, code);
   qname->SetContext(pos);
   return true;
}

//------------------------------------------------------------------------------

void Editor::QualifyReferent(const CxxToken* item, CxxNamed* ref)
{
   Debug::ft("Editor.QualifyReferent");

   //  Within ITEM, prefix NS wherever SYMBOL appears as an identifier.
   //
   Namespace* ns = ref->GetSpace();
   auto symbol = &ref->Name();

   switch(ref->Type())
   {
   case Cxx::Namespace:
      ns = static_cast<Namespace*>(ref)->OuterSpace();
      break;
   case Cxx::Class:
      if(ref->IsInTemplateInstance())
      {
         auto tmplt = ref->GetTemplate();
         ns = tmplt->GetSpace();
         symbol = &tmplt->Name();
      }
   }

   auto name = ns->ScopedName(false);
   auto qual = name + SCOPE_STR;
   size_t pos, end;
   if(!item->GetSpan2(pos, end)) return;
   string id;

   while(FindIdentifier(pos, id, false) && (pos <= end))
   {
      if(id == *symbol)
      {
         //  Qualify ID with NS if it is not already qualified.
         //
         if(code_.rfind(SCOPE_STR, pos) != (pos - strlen(SCOPE_STR)))
         {
            auto elem = file_->PosToItem(pos);
            auto qname = (elem != nullptr ? elem->GetQualName() : nullptr);
            if(qname != nullptr) qname->AddPrefix(name, ns);
            Insert(pos, qual);
            if(qname != nullptr) qname->SetContext(pos);
            Changed();
            pos += qual.size();
         }
      }

      //  Look for the next name after the current one.
      //
      pos += id.size();
   }
}

//------------------------------------------------------------------------------

fn_name Editor_Rename = "Editor.Rename";

void Editor::Rename(size_t pos, const string& oldName, const string& newName)
{
   Debug::ft(Editor_Rename);

   if(pos == string::npos)
   {
      auto expl = "Invalid position for " + oldName;
      Debug::SwLog(Editor_Rename, expl, 0, false);
      return;
   }

   //  This can be invoked when renaming an anonymous item.  It has no name,
   //  so don't try to look for it and change it.
   //
   if(oldName.empty()) return;
   pos = FindWord(pos, oldName);
   if(pos == string::npos) return;
   Replace(pos, oldName.size(), newName);
}

//------------------------------------------------------------------------------

word Editor::RenameArgument(const CodeWarning& log)
{
   Debug::ft("Editor.RenameArgument");

   //  This handles the following argument warnings:
   //  o AnonymousArgument: unnamed argument
   //  o DefinitionRenamesArgument: definition's name differs from declaration's
   //  o OverrideRenamesArgument: override's name differs from root's
   //
   auto func = static_cast<const Function*>(log.item_);
   auto index = func->LogOffsetToArgIndex(log.offset_);
   const Function* decl = func->GetDecl();
   const Function* defn = func->GetDefn();
   const Function* root = (func->IsOverride() ? func->FindRootFunc() : nullptr);

   //  Use the argument name from the root (if any), else the declaration,
   //  else the definition.
   //
   string argName;
   string defnName;
   const auto& declName = decl->GetArgs().at(index)->Name();
   if(defn != nullptr) defnName = defn->GetArgs().at(index)->Name();
   if(root != nullptr) argName = root->GetArgs().at(index)->Name();
   if(argName.empty()) argName = declName;
   if(argName.empty()) argName = defnName;
   if(argName.empty()) return NotFound("Candidate argument name");

   //  The declaration and definition are logged separately, so fix only
   //  the one that has a problem.
   //
   switch(log.warning_)
   {
   case AnonymousArgument:
   {
      auto pos = func->GetArgs().at(index)->GetTypeSpec()->GetPos();
      pos = FindFirstOf(",)", pos);
      if(pos == string::npos) return NotFound("End of argument");
      auto arg = func->GetArgs().at(index).get();
      arg->Rename(argName);
      argName.insert(0, 1, SPACE);
      Insert(pos, argName);
      return Changed(pos);
   }

   case DefinitionRenamesArgument:
      //
      //  See which name the user wants to use.
      //
      if(!declName.empty() && !defnName.empty())
      {
         argName = ChooseArgumentName(declName, defnName);
         if(argName == defnName) func = decl;
      }
   }

   size_t begin, end;
   if(!func->GetSpan2(begin, end)) return NotFound("Function");
   if(func == decl) defnName = declName;
   auto arg = func->GetArgs().at(index).get();
   arg->Rename(argName);
   return func->GetFile()->GetEditor().Changed(begin);
}

//------------------------------------------------------------------------------

word Editor::RenameDebugFtArgument(const CodeWarning& log)
{
   Debug::ft("Editor.RenameDebugFtArgument");

   //  This handles the following warnings for the string passed to Debug::ft:
   //  o DebugFtNameMismatch: the string doesn't start with "Scope.Function"
   //  o DebugFtNameDuplicated: another function already uses the same string
   //
   //  LOG's position is where the Debug::ft call occurs, but log.item_ is
   //  the function in which the Debug::ft call occurs.  Find the Debug::ft
   //  argument, which could be an fn_name or string literal.
   //
   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound("Position of Debug::ft");
   auto lpar = code_.find('(', begin);
   if(lpar == string::npos) return NotFound("Debug::ft left parenthesis");
   auto npos = FindNonBlank(lpar + 1);
   if(npos == string::npos) return NotFound("Start of Debug::ft argument");
   auto arg = file_->PosToItem(npos);
   if(arg == nullptr) return NotFound("Debug::ft argument");

   //  Generate the string (FLIT) and fn_name (FVAR).  If FLIT is already
   //  in use, prompt the user for a unique suffix.
   //
   string flit, fvar, fname;
   auto func = static_cast<const Function*>(log.item_);

   DebugFtNames(func, flit, fvar);
   if(!EnsureUniqueDebugFtName(func, flit, fname))
      return Report(FixSkipped);

   if(arg->Type() == Cxx::StringLiteral)
   {
      //  Replace the string literal in the Debug::ft invocation.
      //
      auto slit = static_cast<StrLiteral*>(arg);
      auto size = slit->GetStr().size();
      auto lpos = code_.find(QUOTE, lpar);
      if(lpos == string::npos) return NotFound("Debug::ft left quote");
      auto rpar = code_.find(')', lpos + size + 1);
      if(rpar == string::npos) return NotFound("Debug::ft right parenthesis");
      Replace(lpos, rpar - lpos, fname);
      slit->Replace(fname.substr(1, fname.size() - 2));
      return Changed(begin);
   }

   //  The Debug::ft invocation used an fn_name.  Replace its definition
   //  and rename it if it doesn't follow the Scope_Function convention.
   //
   auto data = static_cast<SpaceData*>(arg->Referent());
   if(data == nullptr) return NotFound("Debug::ft fn_name");
   auto dpos = data->GetPos();
   auto lpos = Find(dpos, QUOTE_STR);
   if(lpos == string::npos) return NotFound("fn_name left quote");
   auto rpos = code_.find(QUOTE, lpos + 1);
   if(rpos == string::npos) return NotFound("fn_name right quote");
   auto slit = static_cast<StrLiteral*>(file_->PosToItem(lpos));
   Replace(lpos, rpos - lpos + 1, fname);
   if(data->Name() != fvar) data->Rename(fvar);

   if(LineSize(lpos) - 1 > LineLengthMax())
   {
      auto epos = FindFirstOf("=", dpos);
      if(epos != string::npos) InsertLineBreak(epos + 1);
   }

   if(slit != nullptr) slit->Replace(fname.substr(1, fname.size() - 2));
   return Changed(lpos);
}

//------------------------------------------------------------------------------

word Editor::RenameIncludeGuard(const CodeWarning& log)
{
   Debug::ft("Editor.RenameIncludeGuard");

   //  This warning is logged against the #ifndef.
   //
   auto ifn = CurrBegin(log.Pos());
   if(ifn == string::npos) return NotFound("Position of #ifndef");
   if(CompareCode(ifn, HASH_IFNDEF_STR) != 0) return NotFound(HASH_IFNDEF_STR);

   auto guard = log.File()->MakeGuardName();
   static_cast<Ifndef*>(log.item_)->ChangeName(guard);
   auto def = Find(ifn, HASH_DEFINE_STR);
   return Changed(def);
}

//------------------------------------------------------------------------------

size_t Editor::Replace(size_t pos, size_t count, const std::string& code)
{
   Debug::ft("Editor.Replace");

   code_.erase(pos, count);
   code_.insert(pos, code);

   auto size = code.size();

   if(count > size)
      UpdatePos(Erased, pos + size, count - size);
   else
      UpdatePos(Inserted, pos + count, size - count);
   return pos;
}

//------------------------------------------------------------------------------

word Editor::ReplaceHeading(const CodeWarning& log)
{
   Debug::ft("Editor.ReplaceHeading");

   //  Remove the existing header and replace it with the standard one,
   //  inserting the file name where appropriate.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

bool Editor::ReplaceImpl(Function* func) const
{
   Debug::ft("Editor.ReplaceImpl");

   ParserPtr parser(new Parser());
   if(!parser->ReplaceImpl(func, code_)) return false;
   func->GetImpl()->UpdateXref(true);
   return true;
}

//------------------------------------------------------------------------------

word Editor::ReplaceName(const CodeWarning& log)
{
   Debug::ft("Editor.ReplaceName");

   //  Prompt for a new name that will replace the existing one.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::ReplaceNull(const CodeWarning& log)
{
   Debug::ft("Editor.ReplaceNull");

   auto pos = log.Pos();
   auto type = static_cast<TypeName*>(log.item_);
   type->Rename(NULLPTR_STR);
   type->UpdateXref(false);
   type->SetReferent(Singleton<CxxRoot>::Instance()->NullptrTerm(), nullptr);
   type->UpdateXref(true);
   return Changed(pos);
}

//------------------------------------------------------------------------------

word Editor::ReplaceSlashAsterisk(const CodeWarning& log)
{
   Debug::ft("Editor.ReplaceSlashAsterisk");

   auto pos0 = CurrBegin(log.Pos());
   if(pos0 == string::npos) return NotFound("Position of /*");
   auto pos1 = code_.find(COMMENT_BEGIN_STR, pos0);
   if(pos1 == string::npos) return NotFound(COMMENT_BEGIN_STR);
   auto pos2 = LineFind(pos1, COMMENT_END_STR);
   auto pos3 = LineFindNext(pos1 + strlen(COMMENT_BEGIN_STR));
   auto pos4 = (pos2 == string::npos ? string::npos :
      LineFindNext(pos2 + strlen(COMMENT_END_STR)));

   //  We now have
   //  o pos1: start of /*
   //  o pos2: start of */ (if on this line)
   //  o pos3: first non-blank following /* (if any)
   //  o pos4: first non-blank following */ (if any)
   //
   //  The scenarios are                                        pos2 pos3 pos4
   //  1: /*         erase /* and continue                        -    -    -
   //  2: /* aaa     replace /* with // and continue              -    *    -
   //  3: /* aaa */  replace /* with // and erase */ and return   *    *    -
   //  4: /* */ aaa  return                                       *    *    *
   //
   if(pos4 != string::npos)  // [4]
   {
      return Report("Unchanged: code follows /*...*/");
   }
   else if(pos3 == string::npos)  // [1]
   {
      Erase(pos1, strlen(COMMENT_BEGIN_STR));
      Changed();
   }
   else if((pos2 == string::npos) && (pos3 != string::npos))  // [2]
   {
      code_.replace(pos1, strlen(COMMENT_BEGIN_STR), COMMENT_STR);
      Changed();
   }
   else  // [3]
   {
      Erase(pos2, strlen(COMMENT_END_STR));
      code_.replace(pos1, strlen(COMMENT_BEGIN_STR), COMMENT_STR);
      return Changed(pos1);
   }

   //  Subsequent lines will be commented with "//", which will be indented
   //  appropriately and followed by two spaces.
   //
   auto info = GetLineInfo(pos0);
   auto comment = spaces(info->depth * IndentSize());
   comment += COMMENT_STR + spaces(2);

   //  Now continue with subsequent lines:
   //  o pos0: start of the current line
   //  o pos2: start of */ (if on this line)
   //  o pos3: first non-blank after start of line and preceding */
   //  o pos4: first non-blank following */ (if any)
   //
   //  The scenarios are                                    pos2 pos3 pos4
   //  1: no */       prefix // and continue                  -
   //  2: */          erase */ and return                     *    -    -
   //  3: */ aaa      replace */ with line break and return   *    -    *
   //  4: aaa */      prefix // and erase */ and return       *    *    -
   //  5: aaa */ aaa  prefix // and replace */ with line      *    *    *
   //                 break and return
   //
   for(pos0 = NextBegin(pos0); pos0 != string::npos; pos0 = NextBegin(pos0))
   {
      pos2 = LineFind(pos0, COMMENT_END_STR);
      pos3 = LineFindNext(pos0);
      if(pos3 == pos2) pos3 = string::npos;

      if(pos2 != string::npos)
         pos4 = LineFindNext(pos2 + strlen(COMMENT_END_STR));
      else
         pos4 = string::npos;

      if(pos2 == string::npos)  // [1]
      {
         InsertPrefix(pos0, comment);
         Changed();
      }
      else if((pos3 == string::npos) && (pos4 == string::npos))  // [2]
      {
         Erase(pos2, strlen(COMMENT_END_STR));
         return Changed(PrevBegin(pos2));
      }
      else if((pos3 == string::npos) && (pos4 != string::npos))  // [3]
      {
         Erase(pos2, strlen(COMMENT_END_STR));
         Changed();
         InsertLineBreak(pos2);
         return Changed(PrevBegin(pos2));
      }
      else if((pos3 != string::npos) && (pos4 == string::npos))  // [4]
      {
         Erase(pos2, strlen(COMMENT_END_STR));
         InsertPrefix(pos0, comment);
         return Changed(pos0);
      }
      else  // [5]
      {
         Erase(pos2, strlen(COMMENT_END_STR));
         InsertLineBreak(pos2);
         InsertPrefix(pos0, comment);
         return Changed(pos0);
      }
   }

   return Report("Closing */ not found. Inspect changes!");
}

//------------------------------------------------------------------------------

word Editor::ReplaceUsing(const CodeWarning& log)
{
   Debug::ft("Editor.ReplaceUsing");

   //  Before removing the using statement, add type aliases to each class
   //  for symbols that appear in its definition and that were resolved by
   //  a using statement.
   //
   ResolveUsings();
   return EraseItem(log.item_);
}

//------------------------------------------------------------------------------
//
//  Displays EXPL when RC was returned after fixing LOG.
//
void Editor::ReportFix(const CodeWarning* log, word rc)
{
   Debug::ft("Editor.ReportFix");

   auto expl = GetExpl();

   if(rc < EditCompleted)
   {
      *Cli_->obuf << spaces(2) << (expl.empty() ? SuccessExpl : expl);
      if(expl.empty() || (expl.back() != CRLF)) *Cli_->obuf << CRLF;
      Cli_->Flush();
   }

   if((log != nullptr) && (rc == EditSucceeded)) log->status_ = Pending;
}

//------------------------------------------------------------------------------

void Editor::ReportFixInFile(const CodeWarning* log, word rc) const
{
   Debug::ft("Editor.ReportFixInFile");

   auto fn = file_->Name();
   auto expl = GetExpl();

   if(expl.empty())
   {
      fn += ": ";
      expl = fn + SuccessExpl;
   }
   else
   {
      fn += ":\n" + spaces(4);
      expl = fn + expl;
   }

   SetExpl(expl);
   ReportFix(log, rc);
}

//------------------------------------------------------------------------------

word Editor::ResolveUsings()
{
   Debug::ft("Editor.ResolveUsings");

   //  This function only needs to run once per file.
   //
   if(aliased_) return EditSucceeded;

   const auto& items = file_->Items();

   for(auto i = items.begin(); i != items.end(); ++i)
   {
      if((*i)->IsInternal()) continue;

      auto refs = FindUsingReferents(*i);

      for(auto ref = refs.cbegin(); ref != refs.cend(); ++ref)
      {
         QualifyReferent(*i, *ref);
      }
   }

   aliased_ = true;
   return EditSucceeded;
}

//------------------------------------------------------------------------------

word Editor::SortFunctions(const CodeWarning& log)
{
   Debug::ft("Editor.SortFunctions");

   //  Start by getting the functions defined in our file, sorted by position.
   //  Extract the functions in the AREA associated with the log; all of them
   //  will be sorted together.  Step through the file until the first function
   //  in AREA is reached.  Each of its functions will end up in one of three
   //  groups:
   //  o SORTED: functions that don't need to move
   //  o UNSORTED: functions that need to move
   //  o ORPHANS: functions located *after* the first function in another area;
   //    all of these also need to move
   //
   auto defns = file_->GetFuncDefnsToSort();
   auto area = log.item_->GetArea();
   auto reached = false;
   auto orphans = FuncsInArea(defns, area);
   FunctionVector sorted;
   FunctionVector unsorted;
   Function* prev = nullptr;

   for(auto f = defns.cbegin(); f != defns.cend(); ++f)
   {
      if((*f)->GetArea() == area)
      {
         reached = true;

         if((prev == nullptr) || FuncDefnsAreSorted(prev, *f))
         {
            prev = *f;
            sorted.push_back(*f);
         }
         else
         {
            //  When functions were previously sorted, re-sorting is usually
            //  required only because of name changes of the addition of new
            //  functions.  In this case, when f2 and f3 are missorted in the
            //  sequence f1-f2-f3, cut and paste operations can be reduced by
            //  moving f2, provided that f3 follows f1.  Here, that means
            //  moving PREV from SORTED to UNSORTED and adding *f to SORTED.
            //
            auto size = sorted.size();

            if((size > 1) && FuncDefnsAreSorted(sorted[size - 2], *f))
            {
               unsorted.push_back(prev);
               sorted.pop_back();
               prev = *f;
               sorted.push_back(*f);
            }
            else
            {
               unsorted.push_back(*f);
            }
         }

         CodeTools::EraseItem(orphans, *f);
      }
      else if(reached)
      {
         break;
      }
   }

   if(!reached) return NotFound("Unsorted function");

   //  A function in AREA was found, and now we've reached a function in a
   //  different area.  Add any functions in ORPHANS to UNSORTED, and then
   //  move each unsorted function to its correct position.
   //
   for(auto o = orphans.cbegin(); o != orphans.cend(); ++o)
   {
      unsorted.push_back(*o);
   }

   while(!unsorted.empty())
   {
      auto func = unsorted.back();
      unsorted.pop_back();
      MoveFuncDefn(sorted, func);
      sorted.push_back(func);
      std::sort(sorted.begin(), sorted.end(), IsSortedByPos);
   }

   return EditSucceeded;
}

//------------------------------------------------------------------------------

word Editor::SortIncludes()
{
   Debug::ft("Editor.SortIncludes");

   //  Sort our file's #include directives.  This doesn't actually move their
   //  code, so each one retains its pre-sorted position.  This allows us to
   //  then selection sort them.  Find each successive #include, and if its
   //  position doesn't match the next #include in the list, cut the correct
   //  #include and paste it in front of the current one.
   //
   file_->SortIncludes();

   const auto& includes = file_->Includes();
   auto incl = includes.cbegin();

   for(auto pos = IncludesBegin(); pos != string::npos; pos = NextBegin(pos))
   {
      if(CompareCode(pos, HASH_INCLUDE_STR) == 0)
      {
         if(file_->PosToItem(pos) != incl->get())
         {
            auto from = (*incl)->GetPos();
            auto code = GetCode(from);
            from = EraseLine(from);
            Paste(pos, code, from);
         }

         if(++incl == includes.cend()) break;
      }
   }

   sorted_ = true;
   Changed();
   return Report("All #includes in this file sorted.", EditSucceeded);
}

//------------------------------------------------------------------------------

word Editor::SortOverrides(const CodeWarning& log)
{
   Debug::ft("Editor.SortOverrides");

   //  Get the functions for the class associated with LOG, sorted by position.
   //  Within each access control, find all overrides and the LAST function that
   //  is not an override.  Sort the overrides alphabetically and move them so
   //  that they follow LAST.  A function that shares a comment with other items
   //  is not moved, even if it is an override.
   //
   auto cls = log.item_->GetClass();
   auto funcs = cls->GetFunctions();
   Function* last = nullptr;
   FunctionVector overrides;
   size_t index = 0;
   size_t pos = string::npos;

   while(index < funcs.size())
   {
      auto acc = funcs[index]->GetAccess();

      do
      {
         auto func = funcs[index];
         if(func->GetAccess() != acc) break;

         if(IsInItemGroup(func))
            last = func;
         else if(!func->IsOverride())
            last = func;
         else
            overrides.push_back(func);
      }
      while(++index < funcs.size());

      if(!overrides.empty())
      {
         //  Find POS, the insertion point when moving overrides.  If all
         //  of the functions were overrides (LAST is nullptr), POS is at
         //  the start of the first override.  If a non-override was found
         //  (LAST is not nullptr), POS follows LAST.
         //
         if(last == nullptr)
         {
            size_t end;
            GetSpan(overrides.front(), pos, end);
         }
         else
         {
            size_t begin;
            GetSpan(last, begin, pos);
            pos = NextBegin(pos);
         }

         //  Sort the overrides and then cut and paste each one to POS.
         //
         std::sort(overrides.begin(), overrides.end(), IsSortedByName);

         for(size_t i = overrides.size() - 1; i != SIZE_MAX; --i)
         {
            MoveItem(overrides[i], pos, last);
         }

         overrides.clear();
      }

      last = nullptr;
   }

   return Report("All overrides in this class sorted.", EditSucceeded);
}

//------------------------------------------------------------------------------

word Editor::SplitVirtualFunction(const Function* func)
{
   Debug::ft("Editor.SplitVirtualFunction");

   //  Split this public virtual function:
   //  o Rename the function, its overrides, and its invocations within
   //    overrides to its original name + "_v".
   //  o Make its declaration protected and virtual.
   //  o Make its public declaration non-virtual, with the implementation
   //    simply invoking its renamed, protected version.
   //
   return Unimplemented();
}

//------------------------------------------------------------------------------

word Editor::TagAsConstArgument(const Function* func, word offset)
{
   Debug::ft("Editor.TagAsConstArgument");

   //  Find the line on which the argument's type appears, and insert
   //  "const" before that type.
   //
   auto index = func->LogOffsetToArgIndex(offset);
   auto type = func->GetArgs().at(index)->GetTypeSpec();
   auto pos = type->GetPos();
   if(pos == string::npos) return NotFound("Argument type");
   Insert(pos, "const ");
   type->Tags()->SetConst(true);
   return Changed(pos);
}

//------------------------------------------------------------------------------

word Editor::TagAsConstData(const Data* data)
{
   Debug::ft("Editor.TagAsConstData");

   //  Insert "const" before the data declaration's type.
   //
   auto type = data->GetTypeSpec();
   auto pos = type->GetPos();
   if(pos == string::npos) return NotFound("Data type");
   Insert(pos, "const ");
   type->Tags()->SetConst(true);
   return Changed(pos);
}

//------------------------------------------------------------------------------

word Editor::TagAsConstFunction(Function* func)
{
   Debug::ft("Editor.TagAsConstFunction");

   //  Insert " const" after the right parenthesis at the end of the function's
   //  argument list.
   //
   auto endsig = FindArgsEnd(func);
   if(endsig == string::npos) return NotFound("End of argument list");
   Insert(endsig + 1, " const");
   func->SetConst(true);
   return Changed(endsig);
}

//------------------------------------------------------------------------------

word Editor::TagAsConstPointer(const Data* data)
{
   Debug::ft("Editor.TagAsConstPointer");

   //  If there is more than one pointer, this applies to the last one, so
   //  back up from the data item's name.  Search for the name on this line
   //  in case other edits have altered its position.
   //
   auto type = data->GetTypeSpec();
   auto name = FindWord(data->GetPos(), data->Name());
   if(name == string::npos) return NotFound("Member name");
   auto ptr = Rfind(name, "*");
   if(ptr == string::npos) return NotFound("Pointer tag");
   Insert(ptr + 1, " const");
   type->Tags()->SetConstPtr();
   return Changed(ptr);
}

//------------------------------------------------------------------------------

word Editor::TagAsConstReference(const Function* func, word offset)
{
   Debug::ft("Editor.TagAsConstReference");

   //  Find the line on which the argument's name appears.  Insert a reference
   //  tag before the argument's name and "const" before its type.  The tag is
   //  added first so that its position won't change as a result of adding the
   //  "const" earlier in the line.
   //
   const auto& args = func->GetArgs();
   auto index = func->LogOffsetToArgIndex(offset);
   auto arg = args.at(index).get();
   if(arg == nullptr) return NotFound("Argument");
   auto pos = arg->GetPos();
   if(pos == string::npos) return NotFound("Argument name");
   auto prev = RfindNonBlank(pos - 1);
   Insert(prev + 1, "&");
   arg->GetTypeSpec()->Tags()->SetRefs(1);
   auto rc = TagAsConstArgument(func, offset);
   if(rc != EditSucceeded) return rc;
   return Changed(prev);
}

//------------------------------------------------------------------------------

word Editor::TagAsDefaulted(Function* func)
{
   Debug::ft("Editor.TagAsDefaulted");

   //  If this is a separate definition, delete it.
   //
   if(func->GetDecl() != func)
   {
      return EraseItem(func);
   }

   //  This is the function's declaration, and possibly its definition.
   //
   auto endsig = FindSigEnd(func);
   if(endsig == string::npos) return NotFound("Signature end");
   if(code_[endsig] == ';')
   {
      Insert(endsig, " = default");
   }
   else
   {
      auto right = FindFirstOf("}", endsig + 1);
      if(right == string::npos) return NotFound("Right brace");
      Replace(endsig, right - endsig + 1, "= default;");
   }

   func->SetDefaulted();
   return Changed(endsig);
}

//------------------------------------------------------------------------------

word Editor::TagAsExplicit(const CodeWarning& log)
{
   Debug::ft("Editor.TagAsExplicit");

   //  A constructor can be tagged "constexpr", which the parser looks for
   //  only *after* "explicit".
   //
   auto ctor = log.item_->GetPos();
   auto prev = LineRfind(ctor, CONSTEXPR_STR);
   if(prev != string::npos) ctor = prev;
   Insert(ctor, "explicit ");
   static_cast<Function*>(log.item_)->SetExplicit(true);
   return Changed(ctor);
}

//------------------------------------------------------------------------------

word Editor::TagAsNoexcept(Function* func)
{
   Debug::ft("Editor.TagAsNoexcept");

   //  Insert "noexcept" after "const" but before "override" or "final".
   //  Start by finding the end of the function's argument list.
   //
   auto rpar = FindArgsEnd(func);
   if(rpar == string::npos) return NotFound("End of argument list");

   //  If there is a "const" after the right parenthesis, insert "noexcept"
   //  after it, else insert "noexcept" after the right parenthesis.
   //
   auto cons = FindNonBlank(rpar + 1);
   if(CompareCode(cons, CONST_STR) == 0)
   {
      Insert(cons + strlen(CONST_STR), " noexcept");
      return Changed(cons);
   }

   Insert(rpar + 1, " noexcept");
   func->SetNoexcept(true);
   return Changed(rpar);
}

//------------------------------------------------------------------------------

word Editor::TagAsOverride(const CodeWarning& log)
{
   Debug::ft("Editor.TagAsOverride");

   //  Remove the function's virtual tag, if any.
   //
   EraseVirtualTag(log);

   //  Insert "override" after the last non-blank character at the end
   //  of the function's signature.
   //
   auto endsig = FindSigEnd(log);
   if(endsig == string::npos) return NotFound("Signature end");
   endsig = RfindNonBlank(endsig - 1);
   Insert(endsig + 1, " override");
   static_cast<Function*>(log.item_)->SetOverride(true);
   return Changed(endsig);
}

//------------------------------------------------------------------------------

word Editor::TagAsStaticClassFunction(Function* func)
{
   Debug::ft("Editor.TagAsStaticClassFunction");

   //  Start with the function's return type in case it's on the line above
   //  the function's name.  Then find the end of the argument list.
   //
   auto type = func->GetTypeSpec()->GetPos();
   if(type == string::npos) return NotFound("Function return type");
   auto rpar = FindArgsEnd(func);
   if(rpar == string::npos) return NotFound("End of argument list");

   //  Only a function's declaration, not its definition, is tagged "static".
   //  Start by removing any "virtual" tag.  Then put "static" after "extern"
   //  and/or "inline".
   //
   if(func->GetDecl() == func)
   {
      auto front = LineRfind(type, VIRTUAL_STR);
      if(front != string::npos) Erase(front, strlen(VIRTUAL_STR) + 1);
      front = LineRfind(type, INLINE_STR);
      if(front != string::npos) type = front;
      front = LineRfind(type, EXTERN_STR);
      if(front != string::npos) type = front;
      Insert(type, "static ");
      func->SetStatic(true, Cxx::NIL_OPERATOR);
      Changed();
   }

   //  A static function cannot be const, so remove that tag if it exists.  If
   //  "const" is on the same line as RPAR, delete any space *before* "const";
   //  if it's on the next line, delete any space *after* "const" to preserve
   //  indentation.
   //
   if(func->IsConst())
   {
      auto tag = FindWord(rpar, CONST_STR);

      if(tag != string::npos)
      {
         Erase(tag, strlen(CONST_STR));
         func->SetConst(false);

         if(OnSameLine(rpar, tag))
         {
            if(IsBlank(code_[tag - 1])) Erase(tag - 1, 1);
         }
         else
         {
            if(IsBlank(code_[tag])) Erase(tag, 1);
         }
      }
   }

   return Changed(type);
}

//------------------------------------------------------------------------------

word Editor::TagAsStaticData(Data* data)
{
   Debug::ft("Editor.TagAsStaticData");

   //  Insert "static" before the data declaration's type.
   //
   auto type = data->GetTypeSpec();
   auto pos = type->GetPos();
   if(pos == string::npos) return NotFound("Data type");
   Insert(pos, "static ");
   data->SetStatic(true);
   return Changed(pos);
}

//------------------------------------------------------------------------------

word Editor::TagAsStaticFreeFunction(Function* func)
{
   Debug::ft("Editor.TagAsStaticFreeFunction");

   //  Insert "static" before the function's return type.
   //
   auto type = func->GetTypeSpec();
   auto pos = type->GetPos();
   if(pos == string::npos) return NotFound("Function return type");
   Insert(pos, "static ");
   func->SetStatic(true, Cxx::NIL_OPERATOR);
   return Changed(pos);
}

//------------------------------------------------------------------------------

word Editor::TagAsVirtual(const CodeWarning& log)
{
   Debug::ft("Editor.TagAsVirtual");

   //  Make this destructor virtual.
   //
   auto pos = Insert(log.item_->GetPos(), "virtual ");
   static_cast<Function*>(log.item_)->SetVirtual(true);
   return Changed(pos);
}

//------------------------------------------------------------------------------

void Editor::UpdateAfterErase(size_t& pos) const
{
   Debug::ft("Editor.UpdateAfterErase");

   if(pos == string::npos) return;

   if(pos >= lastErasePos_)
   {
      if(pos < (lastErasePos_ + lastEraseSize_))
         pos = string::npos;
      else
         pos -= lastEraseSize_;
   }
}

//------------------------------------------------------------------------------

fn_name Editor_UpdateDebugFt = "Editor.UpdateDebugFt";

void Editor::UpdateDebugFt(Function* func)
{
   Debug::ft(Editor_UpdateDebugFt);

   auto defn = func->GetDefn();
   if(func != defn) return UpdateDebugFt(defn);

   size_t begin, end;
   func->GetSpan2(begin, end);
   auto file = func->GetFile();
   auto& editor = file->GetEditor();
   auto pos = editor.Find(begin, "Debug::ft");

   if(pos < end)
   {
      file->LogPos(pos, DebugFtNameMismatch, func);
      const auto& log = file->GetWarnings().back();
      auto rc = editor.FixLog(log);

      if(rc != EditSucceeded)
      {
         auto expl = "Failed to update " + func->ScopedName(false);
         expl.append(" Debug::ft argument.");
         Debug::SwLog(Editor_UpdateDebugFt, expl, 0, false);
      }
   }
}

//------------------------------------------------------------------------------

word Editor::UpdateItemControls(const Class* cls, ItemDeclAttrs& attrs) const
{
   Debug::ft("Editor.UpdateItemControls");

   //  If the access control for the previous item can be reused, there is
   //  nothing to be done.  If there is a next item, its access control was
   //  taken into account when deciding where to insert the new item.
   //
   auto prevAccess = (attrs.prev_ != nullptr ?
      attrs.prev_->GetAccess() : cls->DefaultAccess());
   if(prevAccess == attrs.access_) return EditContinue;

   //  The previous item's access control isn't the one we want.  But if the
   //  next item's access control is the desired one, the insertion position
   //  has already been adjusted so that it will be acquired.  However, that
   //  position might immediately *follow* an access control,
   //
   auto nextAccess = (attrs.next_ != nullptr ?
      attrs.next_->GetAccess() : prevAccess);
   if(nextAccess == attrs.access_) return EditContinue;

   //  The item's access control is not in effect at the insertion point, so
   //  it must be inserted, and the access control that was in effect must be
   //  inserted after the item unless we've reached the end of the class.
   //
   attrs.thisctrl_ = true;
   if(attrs.blank_ == BlankAbove) attrs.blank_ = BlankBelow;
   if(PosToType(attrs.pos_) != CloseBraceSemicolon)
      attrs.nextctrl_ = nextAccess;
   return EditContinue;
}

//------------------------------------------------------------------------------

word Editor::UpdateItemDeclAttrs
   (const CxxToken* item, ItemDeclAttrs& attrs) const
{
   Debug::ft("Editor.UpdateItemDeclAttrs");

   if(item == nullptr) return EditContinue;

   size_t begin, end;
   if(!item->GetSpan2(begin, end)) return NotFound("Adjacent item");

   //  Indent the code to match that of ITEM.
   //
   attrs.indent_ = LineFindFirst(begin) - CurrBegin(begin);

   //  If ITEM is commented, a blank line should also precede or follow it.
   //
   auto type = PosToType(PrevBegin(begin));
   const auto& line = LineTypeAttr::Attrs[type];

   if(!line.isCode && (type != BlankLine))
   {
      attrs.comment_ = true;
      attrs.blank_ = BlankAbove;
      return EditContinue;
   }

   //  ITEM isn't commented, so see if a blank line precedes and follows it.
   //
   if((type == BlankLine) && (PosToType(NextBegin(end)) == BlankLine))
   {
      attrs.blank_ = BlankAbove;
   }

   return EditContinue;
}

//------------------------------------------------------------------------------

word Editor::UpdateItemDeclLoc(const Class* cls, ItemDeclAttrs& attrs) const
{
   Debug::ft("Editor.UpdateItemDeclLoc");

   //  PREV and NEXT are the items that precede and follow the item whose
   //  declaration is to be inserted.
   //
   auto prev = attrs.prev_;
   auto next = attrs.next_;
   auto rc = UpdateItemDeclAttrs(prev, attrs);
   if(rc != EditContinue) return rc;
   rc = UpdateItemDeclAttrs(next, attrs);
   if(rc != EditContinue) return rc;

   //  Special member functions are added in order of FunctionRole, so add
   //  them bottom-up so that they will appear in the desired order.
   //
   if(next == nullptr)
   {
      //  Insert the item before the class's final closing brace.
      //
      size_t begin, end;
      if(!cls->GetSpan2(begin, end)) return NotFound("Item's class");
      attrs.pos_ = CurrBegin(end);
      if((prev == nullptr) && (next == nullptr)) attrs.comment_ = true;
      if(attrs.blank_ == BlankBelow) attrs.blank_ = BlankAbove;
      return UpdateItemControls(cls, attrs);
   }

   //  Insert the item before NEXT.  If the item is to be set off with a
   //  blank, the blank must follow it.
   //
   if(attrs.blank_ != BlankNone) attrs.blank_ = BlankBelow;

   size_t begin, end;
   if(!next->GetSpan2(begin, end)) return NotFound("Next item's position");
   auto pred = PrevBegin(begin);

   while(true)
   {
      auto type = PosToType(pred);

      if(!LineTypeAttr::Attrs[type].isCode)
      {
         if(type == BlankLine) break;

         //  This is a comment, so keep moving up to find the insertion point.
         //
         pred = PrevBegin(pred);
         continue;
      }

      if(type == AccessControl)
      {
         if(FindAccessControl(GetCode(pred)) != attrs.access_)
         {
            //  This isn't the desired control.  Insert the item before it.
            //
            attrs.pos_ = pred;
            if(attrs.blank_ == BlankBelow) attrs.blank_ = BlankAbove;
            return UpdateItemControls(cls, attrs);
         }

         //  This is the desired access control.  Insert the item after it
         //  so that it can be reused.
         //
         attrs.pos_ = NextBegin(pred);
         return EditContinue;
      }

      break;
   }

   //  Insert the item above the line that follows PRED.
   //
   attrs.pos_ = NextBegin(pred);
   return UpdateItemControls(cls, attrs);
}

//------------------------------------------------------------------------------

void Editor::UpdateItemDefnAttrs(const CxxToken* prev,
   const CxxToken* item, const CxxToken* next, ItemDefnAttrs& attrs) const
{
   Debug::ft("Editor.UpdateItemDefnAttrs");

   auto prevOffsets = GetOffsets(prev);
   auto itemOffsets = GetOffsets(item);
   auto nextOffsets = GetOffsets(next);

   if(item == nullptr)
   {
      //  ITEM doesn't exist, so PREV and NEXT determine the offsets.
      //
      if(prev != nullptr)
      {
         //  The item will be added below PREV, so duplicate the offset
         //  currently below PREV, which will get pushed below the item.
         //
         itemOffsets.above_ = prevOffsets.below_;
      }
      else if(next != nullptr)
      {
         //  The item will be added above NEXT, so duplicate the offset
         //  currently above NEXT, which will get pushed above the item.
         //
         itemOffsets.below_ = nextOffsets.above_;
      }
      else
      {
         //  The item will be added at the end of the file.  There must be
         //  *something* in the file, so add a rule above the item.
         //
         itemOffsets.above_ = OffsetRule;
      }
   }
   else
   {
      //  ITEM already exists, so the offsets above and below it are known.
      //
      if(prev != nullptr)
      {
         //  The offset below PREV will be pushed below ITEM, so clear the
         //  offset below ITEM unless it is greater.  An offset above ITEM
         //  has to be inserted, so use the larger one.
         //
         if(prevOffsets.below_ >= itemOffsets.below_)
            itemOffsets.below_ = OffsetNone;
         if(itemOffsets.above_ < prevOffsets.below_)
            itemOffsets.above_ = prevOffsets.below_;
      }
      else if(next != nullptr)
      {
         //  The offset above NEXT will be pushed above ITEM, so clear the
         //  offset above ITEM unless it is greater.  An offset below ITEM
         //  has to be inserted, so use the larger one.
         //
         if(nextOffsets.above_ >= itemOffsets.above_)
            itemOffsets.above_ = OffsetNone;
         if(itemOffsets.below_ < nextOffsets.above_)
            itemOffsets.below_ = nextOffsets.above_;
      }
   }

   attrs.offsets_.above_ = itemOffsets.above_;
   attrs.offsets_.below_ = itemOffsets.below_;
}

//------------------------------------------------------------------------------

word Editor::UpdateItemDefnLoc(const CxxToken* prev,
   const CxxToken* item, const CxxToken* next, ItemDefnAttrs& attrs) const
{
   Debug::ft("Editor.UpdateItemDefnLoc");

   //  If PREV is NULLPTR and NEXT isn't, ITEM will be inserted before NEXT.
   //  If NEXT is a function that uses items directly above it, insert ITEM
   //  above those items, which are probably specific to NEXT.
   //
   if((prev == nullptr) && (next != nullptr) && (next->Type() == Cxx::Function))
   {
      auto items = GetItemsForDefn(static_cast<const CxxScope*>(next));
      if(!items.empty()) next = items.front();
   }

   UpdateItemDefnAttrs(prev, item, next, attrs);

   if(prev != nullptr)
   {
      //  Insert the item after the one that precedes it.
      //
      attrs.pos_ = LineAfterItem(prev);
      return EditContinue;
   }

   if(next == nullptr)
   {
      //  Insert the item at the bottom of the file.  If it ends with two
      //  closing braces, the second one ends a namespace definition, so
      //  insert the definition above that one.
      //
      auto pos = PrevBegin(string::npos);
      auto type = PosToType(pos);
      if(type != CloseBrace) return NotFound("File's second last }");
      type = PosToType(PrevBegin(pos));
      if(type != CloseBrace) return NotFound("File's last }");
      attrs.pos_ = pos;
      return EditContinue;
   }

   //  Insert the item before NEXT and any blank lines which precede it.
   //
   auto pred = PrevBegin(next->GetPos());

   while(pred != string::npos)
   {
      if(PosToType(pred) == BlankLine)
      {
         pred = PrevBegin(pred);
      }
      else
      {
         attrs.pos_ = NextBegin(pred);
         return EditContinue;
      }
   }

   attrs.pos_ = 0;
   return EditContinue;
}

//------------------------------------------------------------------------------

void Editor::UpdatePos
   (EditorAction action, size_t begin, size_t count, size_t from)
{
   Debug::ft("Editor.UpdatePos");

   switch(action)
   {
   case Erased:
      lastErasePos_ = begin;
      lastEraseSize_ = count;
      break;

   case Pasted:
      lastErasePos_ = string::npos;
      lastEraseSize_ = 0;
      break;
   }

   Update();
   file_->UpdatePos(action, begin, count, from);
   Changed();
}

//------------------------------------------------------------------------------

word Editor::Write() const
{
   Debug::ft("Editor.Write");

   std::ostringstream stream;

   //  Create a new file to hold the reformatted version.
   //
   auto path = file_->Path();
   auto temp = path + ".tmp";
   auto output = FileSystem::CreateOstream(temp.c_str(), true);
   if(output == nullptr)
   {
      stream << "Failed to open output file for " << file_->Name();
      return Report(stream, EditAbort);
   }

   *output << code_;

   //  Delete the original file and replace it with the new one.
   //
   output.reset();
   auto err = remove(path.c_str());

   if(err != 0)
   {
      stream << "Failed to remove " << file_->Name() << ": error=" << err;
      return Report(stream, EditAbort);
   }

   err = rename(temp.c_str(), path.c_str());

   if(err != 0)
   {
      stream << "Failed to rename " << file_->Name() << ": error=" << err;
      return Report(stream, EditAbort);
   }

   const auto& warnings = file_->GetWarnings();

   for(auto w = warnings.begin(); w != warnings.end(); ++w)
   {
      if(w->status_ == Pending) w->status_ = Fixed;
   }

   stream << "..." << file_->Name() << " committed";
   return Report(stream, EditCompleted);
}
}
