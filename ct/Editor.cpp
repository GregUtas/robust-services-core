//==============================================================================
//
//  Editor.cpp
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
#include "Editor.h"
#include <algorithm>
#include <bitset>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iosfwd>
#include <iterator>
#include <list>
#include <memory>
#include <sstream>
#include "CliThread.h"
#include "CodeCoverage.h"
#include "CodeFile.h"
#include "CxxArea.h"
#include "CxxNamed.h"
#include "CxxScope.h"
#include "CxxScoped.h"
#include "CxxString.h"
#include "CxxToken.h"
#include "Debug.h"
#include "Duration.h"
#include "Formatters.h"
#include "Library.h"
#include "NbCliParms.h"
#include "NbTypes.h"
#include "Singleton.h"
#include "SysFile.h"
#include "ThisThread.h"

using std::ostream;
using std::string;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  User prompts.
//
fixed_string YNSQChars = "ynsq";
fixed_string YNSQHelp = "Enter y(yes) n(no) s(skip file) q(quit): ";
fixed_string FixSkipped = "This fix will be skipped.";
fixed_string SuffixPrompt = "Enter a suffix for the name: ";

//  Characters that enclose the file name of an #include directive,
//  depending on the group to which it belongs.
//
const string FrontChars = "$%@!<\"";
const string BackChars = "$%@!>\"";

//------------------------------------------------------------------------------
//
//  Returns an unquoted string (FLIT) and fn_name identifier (FVAR) that are
//  suitable for invoking Debug::ft.
//
void DebugFtNames(const Function* func, string& flit, string& fvar)
{
   Debug::ft("CodeTools.DebugFtNames");

   //  Get the function's name and the scope in which it appears.
   //
   auto sname = *func->GetScope()->Name();
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
//  If CODE is an #include directive, unmangles and returns it, else simply
//  returns it without any changes.
//
string DemangleInclude(string& code)
{
   if(code.find(HASH_INCLUDE_STR) != 0) return code;

   auto front = code.find_first_of(FrontChars);
   if(front == string::npos) return code;
   auto back = rfind_first_not_of(code, code.size() - 1, WhitespaceChars);

   switch(FrontChars.find_first_of(code[front]))
   {
   case 0:
   case 2:
      code[front] = '<';
      code[back] = '>';
      break;
   case 1:
   case 3:
      code[front] = QUOTE;
      code[back] = QUOTE;
      break;
   }

   return code;
}

//------------------------------------------------------------------------------
//
//  Sets FNAME to FLIT, the argument for Debug::ft.  If it is already in use,
//  prompts the user for a suffix to make it unique.  Returns false if FNAME
//  is not unique and the user did not provide a satisfactory suffix, else
//  returns true after enclosing FNAME in quotes.
//
bool EnsureUniqueDebugFtName(CliThread& cli, const string& flit, string& fname)
{
   Debug::ft("CodeTools.EnsureUniqueDebugFtName");

   auto cover = Singleton< CodeCoverage >::Instance();
   fname = flit;

   while(cover->Defined(fname))
   {
      std::ostringstream stream;
      stream << fname << " is already in use. " << SuffixPrompt;
      auto suffix = cli.StrPrompt(stream.str());
      if(suffix.empty()) return false;
      fname = flit + '(' + suffix + ')';
   }

   fname.insert(0, 1, QUOTE);
   fname.push_back(QUOTE);
   return true;
}

//------------------------------------------------------------------------------

fixed_string FilePrompt = "Enter the filename in which to define";

CodeFile* FindFuncDefnFile(CliThread& cli, const Class* cls, const string& name)
{
   Debug::ft("CodeTools.FindFuncDefnFile");

   //  Look at all the functions in the class to which the new function
   //  will be added.  If all of them are implemented in the same file,
   //  define the new function in that file, otherwise ask the user to
   //  specify which file should contain the function.
   //
   std::set< CodeFile* > impls;
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
      prompt << FilePrompt << CRLF << spaces(2);
      prompt << *cls->Name() << SCOPE_STR << name;
      prompt << " ('s' to skip this item): ";
      auto fileName = cli.StrPrompt(prompt.str());
      if(fileName == "s") return nullptr;

      file = Singleton< Library >::Instance()->FindFile(fileName);
      if(file == nullptr)
      {
         *cli.obuf << "  That file is not in the code library.";
         cli.Flush();
      }
   }

   return file;
}

//------------------------------------------------------------------------------
//
//  Adds FUNC and its overrides to FUNCS.
//
void GetOverrides(const Function* func, std::vector< const Function* >& funcs)
{
   Debug::ft("CodeTools.GetOverrides");

   funcs.push_back(func);

   auto defn = static_cast< const Function* >(func->GetMate());
   if((defn != nullptr) && (defn != func)) funcs.push_back(defn);

   auto& overs = func->GetOverrides();

   for(auto f = overs.cbegin(); f != overs.cend(); ++f)
   {
      GetOverrides(*f, funcs);
   }
}

//------------------------------------------------------------------------------
//
//  Returns true if the #include in LINE1 should precede that in LINE2.
//
bool IncludesAreSorted(const string& line1, const string& line2)
{
   //  #includes are sorted by group, then alphabetically.  The characters
   //  that enclose the filename distinguish the groups: [] for group 1,
   //  () for group 2, <> for group 3, and "" for group 4.
   //
   auto pos1 = line1.find_first_of(FrontChars);
   auto pos2 = line2.find_first_of(FrontChars);

   if(pos2 == string::npos)
   {
      if(pos1 == string::npos) return (&line1 < &line2);
      return true;
   }
   else if(pos1 == string::npos) return false;

   auto c1 = line1[pos1];
   auto c2 = line2[pos2];
   auto group1 = FrontChars.find(c1);
   auto group2 = FrontChars.find(c2);
   if(group1 < group2) return true;
   if(group1 > group2) return false;
   auto cmp = strCompare(line1, line2);
   if(cmp < 0) return true;
   if(cmp > 0) return false;
   return (&line1 < &line2);
}

//------------------------------------------------------------------------------
//
//  Sets EXPL to "TEXT not found."  If QUOTES is set, TEXT is enclosed in
//  quotes.  Returns 0.
//
word NotFound(string& expl, fixed_string text, bool quotes = false)
{
   if(quotes) expl = QUOTE;
   expl += text;
   if(quotes) expl.push_back(QUOTE);
   expl += " not found.";
   expl.push_back(CRLF);
   return 0;
}

//------------------------------------------------------------------------------
//
//  Sets EXPL to TEXT and returns RC.
//
word Report(string& expl, fixed_string text, word rc = 0)
{
   expl = text;
   if(expl.back() != CRLF) expl.push_back(CRLF);
   return rc;
}

//------------------------------------------------------------------------------
//
//  Sets EXPL to STREAM and returns RC.
//
word Report(string& expl, const std::ostringstream& stream, word rc = 0)
{
   expl = stream.str();
   if(expl.back() != CRLF) expl.push_back(CRLF);
   return rc;
}

//------------------------------------------------------------------------------
//
//  Displays EXPL when RC was returned after fixing a single item.
//
void ReportFix(CliThread& cli, word rc, string& expl)
{
   if(rc <= 0)
   {
      *cli.obuf << spaces(2) << (expl.empty() ? SuccessExpl : expl);
      if(expl.empty() || (expl.back() != CRLF)) *cli.obuf << CRLF;
      cli.Flush();
   }

   expl.clear();
}

//------------------------------------------------------------------------------
//
//  Returns CODE, indented to LEVEL standard indentations.
//
string strCode(const string& code, size_t level)
{
   return spaces(level * INDENT_SIZE) + code + CRLF;
}

//------------------------------------------------------------------------------
//
//  Returns TEXT, prefixed by "//  " and indented to LEVEL standard
//  indentations.
//
string strComment(const string& text, size_t level)
{
   return spaces(level * INDENT_SIZE) + "//  " + text + CRLF;
}

//------------------------------------------------------------------------------
//
//  Returns the string "//" followed by repetitions of C to fill out the line.
//
string strRule(char c)
{
   string rule = COMMENT_STR;
   return rule.append(LINE_LENGTH_MAX - 2, c);
}

//------------------------------------------------------------------------------
//
//  Invoked when fixing a warning still needs to be implemented.
//
word Unimplemented(string& expl)
{
   return Report(expl, "Fixing this warning is not yet implemented.", -1);
}

//==============================================================================
//
//  Indicates where a blank line should be added when inserting new code.
//
enum BlankLocation
{
   BlankNone,
   BlankBefore,
   BlankAfter
};

//------------------------------------------------------------------------------
//
//  Attributes when inserting a function declaration.
//
struct FuncDeclAttrs
{
   Cxx::Access access;   // desired access control
   BlankLocation blank;  // where to insert a blank line
   bool comment;         // whether to include a comment

   explicit FuncDeclAttrs(Cxx::Access acc) :
      access(acc), blank(BlankNone), comment(false) { }
};

//------------------------------------------------------------------------------
//
//  Attributes when inserting a function definition.
//
struct FuncDefnAttrs
{
   BlankLocation blank;  // where to insert a blank line
   bool rule;            // whether to insert a rule

   FuncDefnAttrs() : blank(BlankNone), rule(false) { }
};

//==============================================================================

std::set< Editor* > Editor::Editors_ = std::set< Editor* >();
size_t Editor::Commits_ = 0;

//------------------------------------------------------------------------------

Editor::Editor() :
   file_(nullptr),
   sorted_(false),
   aliased_(false),
   lastCut_(string::npos)
{
   Debug::ft("Editor.ctor");
}

//------------------------------------------------------------------------------

word Editor::AdjustLineIndentation(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.AdjustLineIndentation");

   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound(expl, "Position for indentation");
   Indent(begin);
   return Changed(begin, expl);
}

//------------------------------------------------------------------------------

word Editor::AdjustTags(const CodeWarning& log, string& expl)
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

   for(auto pos = source_.find(tag, log.Pos()); pos < stop;
      pos = source_.find(tag, pos + 1))
   {
      if(IsBlank(source_[pos - 1]))
      {
         auto prev = RfindNonBlank(pos - 1);
         auto count = pos - prev - 1;
         Erase(prev + 1, count);
         pos -= count;

         //  If the character after the tag is the beginning of an identifier,
         //  insert a space.
         //
         if(ValidFirstChars.find(source_[pos + 1]) != string::npos)
         {
            Insert(pos + 1, SPACE_STR);
         }

         changed = true;
         break;
      }
   }

   if(changed) return Changed(log.Pos(), expl);

   string target = "Detached ";
   target.push_back(tag);
   target.push_back(SPACE);
   return NotFound(expl, target.c_str());
}

//------------------------------------------------------------------------------

word Editor::AlignArgumentNames(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.AlignArgumentNames");

   //  This handles the following argument warnings:
   //  o AnonymousArgument: unnamed argument
   //  o DefinitionRenamesArgument: definition's name differs from declaration's
   //  o OverrideRenamesArgument: override's name differs from root's
   //
   auto func = static_cast< const Function* >(log.item_);
   const Function* decl = func->GetDecl();
   const Function* defn = func->GetDefn();
   const Function* root = (func->IsOverride() ? func->FindRootFunc() : nullptr);
   if(decl == nullptr) return NotFound(expl, "Function's declaration");

   //  Use the argument name from the root (if any), else the declaration,
   //  else the definition.
   //
   auto index = decl->LogOffsetToArgIndex(log.offset_);
   string argName;
   auto declName = *decl->GetArgs().at(index)->Name();
   string defnName;
   if(defn != nullptr) defnName = *defn->GetArgs().at(index)->Name();
   if(root != nullptr) argName = *root->GetArgs().at(index)->Name();
   if(argName.empty()) argName = declName;
   if(argName.empty()) argName = defnName;
   if(argName.empty()) return NotFound(expl, "Candidate argument name");

   //  The declaration and definition are logged separately, so fix only
   //  the one that has a problem.
   //
   if(log.warning_ == AnonymousArgument)
   {
      auto type = func->GetArgs().at(index)->GetTypeSpec()->GetPos();
      type = FindFirstOf(type, ",)");
      if(type == string::npos) return NotFound(expl, "End of argument");
      argName.insert(0, 1, SPACE);
      Insert(type, argName);
      return Changed(type, expl);
   }

   size_t begin, end;
   func->GetRange(begin, end);
   if(begin == string::npos) return NotFound(expl, "Function name");
   if(end == string::npos) return NotFound(expl, "End of function");
   if(func == decl) defnName = declName;

   for(auto pos = FindWord(begin, defnName); pos < end;
      pos = FindWord(pos + 1, defnName))
   {
      Replace(pos, defnName.size(), argName);
      end = end + argName.size() - defnName.size();
      Changed();
   }

   return Changed(begin, expl);
}

//------------------------------------------------------------------------------

word Editor::ChangeAccess
   (const CodeWarning& log, Cxx::Access acc, string& expl)
{
   Debug::ft("Editor.ChangeAccess");

   //  Move the item and insert the access control keyword if necessary.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::ChangeClassToNamespace(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ChangeClassToNamespace");

   //  Replace "class" with "namespace" and "static" with "extern" (for data)
   //  or nothing (for functions).  Delete things that are no longer needed:
   //  base class, access controls, special member functions, and closing ';'.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::ChangeClassToStruct(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ChangeClassToStruct");

   //  Look for the class's name and then back up to "class".
   //
   auto pos = log.item_->GetPos();
   if(pos == string::npos) return NotFound(expl, "Class name");
   pos = Rfind(pos, CLASS_STR);
   if(pos == string::npos) return NotFound(expl, CLASS_STR, true);
   Replace(pos, strlen(CLASS_STR), STRUCT_STR);

   //  If the class began with a "public:" access control, erase it.
   //
   auto left = Find(pos, "{");
   if(left == string::npos) return NotFound(expl, "Left brace");
   auto access = FindWord(left + 1, PUBLIC_STR);
   if(access != string::npos)
   {
      auto colon = FindNonBlank(access + strlen(PUBLIC_STR));
      Erase(colon, 1);
      Erase(access, strlen(PUBLIC_STR));
      if(IsBlankLine(access)) EraseLine(access);
   }
   return Changed(pos, expl);
}

//------------------------------------------------------------------------------

word Editor::Changed()
{
   Debug::ft("Editor.Changed");

   Editors_.insert(this);
   return 0;
}

//------------------------------------------------------------------------------

word Editor::Changed(size_t pos, string& expl)
{
   Debug::ft("Editor.Changed(pos)");

   auto code = GetCode(pos);
   expl = (IsBlankLine(pos) ? EMPTY_STR : code);
   Editors_.insert(this);
   return 0;
}

//------------------------------------------------------------------------------

word Editor::ChangeDebugFtName
   (CliThread& cli, const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ChangeDebugFtName");

   //  This handles the following warnings for the string passed to Debug::ft:
   //  o DebugFtNameMismatch: the string doesn't start with "Scope.Function"
   //  o DebugFtNameDuplicated: another function already uses the same string
   //
   //  Start by finding the location of the logged Debug::ft invocation.
   //
   auto cpos = Find(log.Pos(), "Debug::ft");
   if(cpos == string::npos) return NotFound(expl, "Debug::ft invocation");

   //  Find the location of the first fn_name definition that precedes this
   //  function.  If one is found, it belongs to a previous function if a
   //  right brace appears between it and the start of this function.
   //
   auto fpos = log.item_->GetPos();
   if(fpos == string::npos) return NotFound(expl, "Function name");
   auto dpos = Rfind(fpos, "fn_name");

   if(dpos != string::npos)
   {
      auto valid = true;

      for(auto left = dpos; valid && (left < fpos); left = NextBegin(left))
      {
         if(LineFind(left, "}") != string::npos) valid = false;
      }

      if(!valid) dpos = string::npos;
   }

   //  Generate the string (FLIT) and fn_name (FVAR).  If FLIT is already
   //  in use, prompt the user for a unique suffix.
   //
   string flit, fvar, fname;

   DebugFtNames(static_cast< const Function* >(log.item_), flit, fvar);
   if(!EnsureUniqueDebugFtName(cli, flit, fname))
      return Report(expl, FixSkipped);

   if(dpos == string::npos)
   {
      //  An fn_name definition was not found, so the Debug::ft invocation
      //  must have used a string literal.  Replace it.
      //
      auto lpar = FindFirstOf(cpos, "(");
      if(lpar == string::npos) return NotFound(expl, "Left parenthesis");
      auto rpar = FindClosing('(', ')', lpar + 1);
      if(rpar == string::npos) return NotFound(expl, "Right parenthesis");
      Erase(lpar + 1, rpar - lpar - 1);
      Insert(lpar + 1, fname);
      return Changed(cpos, expl);
   }

   //  The Debug::ft invocation used an fn_name.  It might be used elsewhere
   //  (e.g. for calls to Debug::SwLog), so keep its name and only replace
   //  its definition.
   //
   auto lpos = Find(dpos, QUOTE_STR);
   if(lpos == string::npos) return NotFound(expl, "fn_name left quote");
   auto rpos = source_.find(QUOTE, lpos + 1);
   if(rpos == string::npos) return NotFound(expl, "fn_name right quote");
   Replace(lpos, rpos - lpos + 1, fname);

   if(LineSize(lpos) - 1 > file_->LineLengthMax())
   {
      auto epos = FindFirstOf(dpos, "=");
      if(epos != string::npos) InsertLineBreak(epos + 1);
   }

   return Changed(lpos, expl);
}

//------------------------------------------------------------------------------

word Editor::ChangeFunctionToFree(const Function* func, string& expl)
{
   Debug::ft("Editor.ChangeFunctionToFree");

   //  o If the function is invoked externally, move its declaration out of
   //    its class, into the enclosing namespace, else just erase it.
   //  o In the definition, replace the class name with the namespace in the
   //    signature and fn_name or Debug::ft string literal.  If it uses any
   //    static items from the class, prefix the class name to those items.
   //  o Move the definition to the correct location.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::ChangeFunctionToMember
   (const Function* func, word offset, string& expl)
{
   Debug::ft("Editor.ChangeFunctionToMember");

   //  o Declare the function in the class associated with the argument at
   //    OFFSET, removing that argument.
   //  o Define the function in the correct location, changing the argument
   //    at OFFSET to "this".
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::ChangeInvokerToFree(const Function* func, string& expl)
{
   Debug::ft("Editor.ChangeInvokerToFree");

   //  Change invokers of this function to invoke it directly instead of
   //  through its class.  An invoker not in the same namespace may have
   //  to prefix the function's namespace.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::ChangeInvokerToMember
   (const Function* func, word offset, string& expl)
{
   Debug::ft("Editor.ChangeInvokerToMember");

   //  Change invokers of this function to invoke it through the argument at
   //  OFFSET instead of directly.  An invoker may need to #include the class's
   //  header.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::ChangeOperator(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ChangeOperator");

   //  This fixes two different warnings:
   //  o StaticFunctionViaMember: change . or -> to :: and what precedes
   //    the operator to the name of the function's class.
   //  o BitwiseOperatorOnBoolean: replace | with || or & with &&.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::ChangeStructToClass(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ChangeStructToClass");

   //  Look for the struct's name and then back up to "struct".
   //
   auto pos = log.item_->GetPos();
   if(pos == string::npos) return NotFound(expl, "Struct name");
   pos = Rfind(pos, STRUCT_STR);
   if(pos == string::npos) return NotFound(expl, STRUCT_STR, true);
   Replace(pos, strlen(STRUCT_STR), CLASS_STR);

   //  Unless the struct began with a "public:" access control, insert one.
   //
   auto left = Find(pos, "{");
   if(left == string::npos) return NotFound(expl, "Left brace");
   auto access = FindWord(left + 1, PUBLIC_STR);
   if(access == string::npos)
   {
      string control(PUBLIC_STR);
      control.push_back(':');
      InsertLine(left, control);
   }
   return Changed(pos, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_CodeBegin = "Editor.CodeBegin";

size_t Editor::CodeBegin()
{
   Debug::ft(Editor_CodeBegin);

   std::vector< size_t > positions;

   auto c = file_->Classes();
   if(!c->empty()) positions.push_back(c->front()->GetPos());

   auto d = file_->Datas();
   if(!d->empty()) positions.push_back(d->front()->GetPos());

   auto e = file_->Enums();
   if(!e->empty()) positions.push_back(e->front()->GetPos());

   auto f = file_->Funcs();
   if(!f->empty()) positions.push_back(f->front()->GetPos());

   auto t = file_->Types();
   if(!t->empty()) positions.push_back(t->front()->GetPos());

   size_t pos = string::npos;

   for(auto p = positions.cbegin(); p != positions.end(); ++p)
   {
      if(*p < pos) pos = *p;
   }

   auto ns = false;

   for(pos = PrevBegin(pos); pos != 0; pos = PrevBegin(pos))
   {
      auto type = GetLineType(pos);

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
         //  [[fallthrough]]
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
   for(pos = NextBegin(pos); pos != string::npos; pos = NextBegin(pos))
   {
      auto type = GetLineType(pos);

      if(LineTypeAttr::Attrs[type].isExecutable) return true;
      if(LineTypeAttr::Attrs[type].isBlank) continue;
      if(type == AccessControl) continue;
      return false;
   }

   return false;
}

//------------------------------------------------------------------------------

bool Editor::Commit(const CliThread& cli, string& expl)
{
   Debug::ft("Editor.Commit");

   //  Perform an automatic >format on each file.  In particular, some edits
   //  could have introduced blank line pairs.
   //
   auto err = false;

   while(!Editors_.empty())
   {
      auto editor = Editors_.cbegin();
      if((*editor)->Format(expl) != 0)
         err = true;
      else
         ++Commits_;
      *cli.obuf << spaces(2) << expl;
      expl.clear();
      Editors_.erase(editor);
   }

   return !err;
}

//------------------------------------------------------------------------------

word Editor::ConvertTabsToBlanks()
{
   Debug::ft("Editor.ConvertTabsToBlanks");

   auto indent = file_->IndentSize();
   string tab(1, TAB);

   //  Run through the source, looking for tabs.
   //
   for(size_t begin = 0; begin != string::npos; begin = NextBegin(begin))
   {
      auto pos = LineFind(begin, tab);

      if(pos != string::npos)
      {
         //  A tab has been found.  Replace it with the number of spaces
         //  needed to reach the next tab stop, namely the next multiple
         //  of INDENT.  This code doesn't bother to skip over character
         //  and string literals, which should use \t or TAB or something
         //  that is actually visible.
         //
         for(NO_OP; source_[pos] != CRLF; ++pos)
         {
            if(source_[pos] == TAB)
            {
               auto count = (pos - begin) % indent;
               if(count == 0) count = indent;
               Erase(pos, 1);
               Insert(pos, spaces(count));
            }
         }

         Changed();
      }
   }

   return 0;
}

//------------------------------------------------------------------------------

size_t Editor::CutCode(const CxxNamed* item, string& expl, string& code)
{
   Debug::ft("Editor.CutCode");

   code.clear();

   if(item == nullptr)
   {
      Report(expl, "Internal error: no item specified.");
      return string::npos;
   }

   //  Find the where the code to be cut begins and ends.
   //
   auto begin = FindCutBegin(item);
   if(begin == string::npos)
   {
      NotFound(expl, "Start of code to be edited");
      return string::npos;
   }

   auto end = begin;

   auto endchars = item->EndChars();
   if(endchars.empty())
   {
      Report(expl, "Internal error: item cannot be edited.");
      return string::npos;
   }
   else if(endchars == CRLF_STR)
   {
      end = CurrEnd(begin);
   }
   else
   {
      auto start = begin;

      if((item->Type() == Cxx::Function) && (endchars.find('}') != string::npos))
      {
         //  To find the right brace at the end of a function definition, the
         //  search must start after its initial left brace.
         //
         start = FindFirstOf(begin, "{");
         ++start;
      }

      end = FindFirstOf(start, endchars);
      if(end == string::npos)
      {
         NotFound(expl, "End of code to be edited");
         return string::npos;
      }

      //  See if the character that precedes the item should be cut instead of
      //  the one that terminated it.  If cutting that character, replace it
      //  with a space if a comment follows, to keep trailing comments aligned.
      //  Otherwise erase it.  Finally, adjust END to the previous character or,
      //  if it is the first non-blank character on its line, to the end of the
      //  previous line.
      //
      auto beginchars = item->BeginChars(source_[end]);

      if(!beginchars.empty())
      {
         auto prev = RfindFirstOf(begin - 1, beginchars);

         if(FindComment(prev) != string::npos)
            source_[prev] = SPACE;
         else
            Erase(prev, 1);

         if(IsFirstNonBlank(end))
            end = CurrBegin(end) - 1;
         else
            --end;
      }

      //  When the code ends at a right brace, also cut any semicolon that
      //  immediately follows.
      //
      if(source_[end] == '}')
      {
         Reposition(end + 1);
         if(CurrChar() == ';') end = Curr();
      }

      //  Cut any comment or whitespace that follows on the last line.
      //
      if(NoCodeFollows(end + 1)) end = CurrEnd(end);
   }

   //  If entire lines of code that aren't immediately followed by more code
   //  are being cut, also cut any comment that precedes the code, since it
   //  almost certainly refers only to the code being cut.  But don't do this
   //  for a preprocessor directive (endchars == CRLF_STR).
   //
   if((begin == CurrBegin(begin)) && (end == CurrEnd(end)))
   {
      if(!CodeFollowsImmediately(end) && (endchars != CRLF_STR))
      {
         begin = IntroStart(begin, false);
      }
   }

   //  Extract the code bounded by [begin, end].  If BEGIN and END are not
   //  on the same line, then
   //  o if BEGIN is not at the start of its line, insert a CRLF before it
   //  o if END is not at the end of its line, insert a CRLF after it (and
   //    include it in the code) and re-indent that line
   //
   if(!OnSameLine(begin, end))
   {
      if(source_[begin - 1] != CRLF)
      {
         Insert(begin, CRLF_STR);
         ++begin;
         ++end;
      }

      if(source_[end] != CRLF)
      {
         auto indent = CRLF_STR + spaces(end - CurrBegin(end));
         Insert(end + 1, indent);
         ++end;
      }
   }

   code = source_.substr(begin, end - begin + 1);
   Erase(begin, end - begin + 1);
   return begin;
}

//------------------------------------------------------------------------------

string Editor::DebugFtCode(const string& fname) const
{
   Debug::ft("Editor.DebugFtCode");

   auto call = string(file_->IndentSize(), SPACE) + "Debug::ft(";
   call.append(fname);
   call.append(");");
   return call;
}

//------------------------------------------------------------------------------

void Editor::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Lexer::Display(stream, prefix, options);

   stream << prefix << "file     : " <<
      (file_ != nullptr ? file_->Name() : "no file specified") << CRLF;
   stream << CRLF;
   stream << prefix << "sorted   : " << sorted_ << CRLF;
   stream << prefix << "aliased  : " << aliased_ << CRLF;
   stream << prefix << "warnings : " << warnings_.size() << CRLF;

   if(!options.test(DispVerbose)) return;

   stream << prefix << "source : " << CRLF;

   const auto& info = GetLinesInfo();

   for(auto i = info.cbegin(); i != info.cend(); ++i)
   {
      i->Display(stream);
      auto type = GetLineType(i->begin);
      stream << LineTypeAttr::Attrs[type].symbol << SPACE;
      stream << SPACE << GetCode(i->begin);
   }
}

//------------------------------------------------------------------------------

bool Editor::DisplayLog(const CliThread& cli, const CodeWarning& log, bool file) const
{
   Debug::ft("Editor.DisplayLog");

   if(file)
   {
      *cli.obuf << log.File()->Name() << ':' << CRLF;
   }

   //  Display LOG's details.
   //
   *cli.obuf << "  Line " << log.Line() + 1;
   if(log.offset_ > 0) *cli.obuf << '/' << log.offset_;
   *cli.obuf << ": " << Warning(log.warning_);
   if(log.HasInfoToDisplay()) *cli.obuf << ": " << log.info_;
   *cli.obuf << CRLF;

   if(log.HasCodeToDisplay())
   {
      //  Display the current version of the code associated with LOG.
      //
      *cli.obuf << spaces(2);
      auto code = GetCode(log.Pos());

      if(code.empty())
      {
         *cli.obuf << "Code not found." << CRLF;
         return false;
      }

      if(code.find_first_not_of(WhitespaceChars) == string::npos)
      {
         *cli.obuf << "[line contains only whitespace]" << CRLF;
         return true;
      }

      *cli.obuf << DemangleInclude(code);
   }

   return true;
}

//------------------------------------------------------------------------------

size_t Editor::Erase(size_t pos, size_t count)
{
   Debug::ft("Editor.Erase");

   source_.erase(pos, count);
   lastCut_ = pos;
   Update();
   file_->UpdatePos(Erased, pos, count);
   UpdateWarnings(Erased, pos, count);
   Changed();
   return pos;
}

//------------------------------------------------------------------------------

word Editor::EraseAccessControl(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseAccessControl");

   //  The parser logs RedundantAccessControl at the position
   //  where it occurred; log.item_ is nullptr in this warning.
   //
   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound(expl, "Access control position");
   size_t len = 0;

   //  Look for the access control keyword and note its length.
   //
   auto access = FindWord(begin, PUBLIC_STR);

   while(true)
   {
      if(access != string::npos)
      {
         len = strlen(PUBLIC_STR);
         break;
      }

      access = FindWord(access, PROTECTED_STR);

      if(access != string::npos)
      {
         len = strlen(PROTECTED_STR);
         break;
      }

      access = FindWord(access, PRIVATE_STR);

      if(access != string::npos)
      {
         len = strlen(PRIVATE_STR);
         break;
      }

      return NotFound(expl, "Access control keyword");
   }

   //  Look for the colon that follows the keyword.
   //
   auto colon = FindNonBlank(access + len);
   if((colon == string::npos) || (source_[colon] != ':'))
      return NotFound(expl, "Colon after access control");

   //  Erase the keyword and colon.
   //
   Erase(colon, 1);
   Erase(access, len);
   return Changed(access, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseAdjacentSpaces(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseAdjacentSpaces");

   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound(expl, "Adjacent spaces position");
   auto pos = LineFindFirst(begin);
   if(pos == string::npos) return 0;

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
      while(IsBlank(source_[stop - 1])) --stop;
   else
      stop = CurrEnd(begin);

   cpos = stop;  // (comm - stop) will be number of erased spaces

   while(pos + 1 < stop)
   {
      if(IsBlank(source_[pos]) && IsBlank(source_[pos + 1]))
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
   return Changed(begin, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseArgument(const Function* func, word offset, string& expl)
{
   Debug::ft("Editor.EraseArgument");

   //  In this function invocation, erase the argument at OFFSET.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::EraseBlankLine(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseBlankLine");

   //  Remove the specified line of code.
   //
   EraseLine(log.Pos());
   return Changed();
}

//------------------------------------------------------------------------------

word Editor::EraseBlankLinePairs()
{
   Debug::ft("Editor.EraseBlankLinePairs");

   size_t b1 = 0;

   for(auto b2 = NextBegin(b1); b2 != string::npos; b2 = NextBegin(b1))
   {
      if(IsBlankLine(b1) && (IsBlankLine(b2) || (GetLineType(b1) == EmptyComment)))
      {
         //  Delete a blank line that was preceded by a blank line or an
         //  empty comment.
         //
         EraseLine(b2);
         Changed();
         if(b2 >= source_.size()) return 0;
      }
      else
      {
         b1 = b2;
      }
   }

   return 0;
}

//------------------------------------------------------------------------------

word Editor::EraseClass(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseClass");

   //  Erase the class's definition and the definitions of its functions
   //  and static data.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::EraseCode(const CxxNamed* item, string& expl)
{
   Debug::ft("Editor.EraseCode");

   string code;
   CutCode(item, expl, code);
   if(expl.empty()) return Changed();
   return -1;
}

//------------------------------------------------------------------------------

word Editor::EraseConst(const CodeWarning& log, string& expl)
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
      switch(source_[prev])
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

      return Changed(pos, expl);
   }

   return NotFound(expl, "Redundant const");
}

//------------------------------------------------------------------------------

word Editor::EraseData
   (const CliThread& cli, const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseData");

   auto decl = static_cast< const Data* >(log.item_);
   auto defn = decl->GetMate();
   auto& refs = decl->Xref();

   //  Erase any references to the data.
   //
   for(auto r = refs.cbegin(); r != refs.cend(); ++r)
   {
      auto file = (*r)->GetFile();
      auto pos = (*r)->GetPos();

      if((file == decl->GetFile()) && (pos == decl->GetPos()))
         continue;

      if(defn != nullptr)
      {
         if((file == defn->GetFile()) && (pos == defn->GetPos()))
            continue;
      }

      auto& editor = file->GetEditor();

      if(expl.empty())
      {
         editor.EraseCode(*r, expl);
         if(!expl.empty())
         {
            if(expl.back() == CRLF) expl.pop_back();
            expl += " (" + file->Name() + ", line ";
            expl += std::to_string(file->GetLexer().GetLineNum(pos)) += ")\n";
         }
      }

      if(!expl.empty())
      {
         *cli.obuf << spaces(2) << expl << CRLF;
         expl.clear();
      }
   }

   //  Erase the data definition, if any.
   //
   if((defn != nullptr) && (defn != decl))
   {
      auto& editor = defn->GetFile()->GetEditor();

      editor.EraseCode(defn, expl);
      if(!expl.empty())
      {
         expl = "Failed to remove definition";
         *cli.obuf << spaces(2) << expl << CRLF;
      }
   }

   //  Erase the data declaration.
   //
   return EraseCode(log.item_, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseDefault(const Function* func, word offset, string& expl)
{
   Debug::ft("Editor.EraseDefault");

   //  Erase this argument's default value.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::EraseEmptyNamespace(size_t pos)
{
   Debug::ft("Editor.EraseEmptyNamespace");

   //  POS is the character after a forward declaration that was just
   //  deleted.  If this left an empty "namespace <ns> { }", remove it.
   //
   if(pos == string::npos) return 0;
   if(!CodeMatches(pos, "}")) return 0;

   auto p1 = PrevBegin(pos);
   if(p1 == 0) return 0;
   auto p2 = PrevBegin(p1);

   if(CodeMatches(p2, NAMESPACE_STR) && (source_[p1] == '{'))
   {
      auto end = CurrEnd(pos);
      Erase(p2, end - p2 + 1);
      return Changed();
   }

   return 0;
}

//------------------------------------------------------------------------------

word Editor::EraseEmptySeparators()
{
   Debug::ft("Editor.EraseEmptySeparators");

   size_t b1 = 0;
   auto b2 = NextBegin(b1);
   if(b2 == string::npos) return 0;

   for(auto b3 = NextBegin(b2); b3 != string::npos; b3 = NextBegin(b2))
   {
      auto t1 = GetLineType(b1);
      auto t2 = GetLineType(b2);
      auto t3 = GetLineType(b3);

      if(LineTypeAttr::Attrs[t2].isBlank)
      {
         if(t1 == SeparatorComment)
         {
            if((t3 == CloseBrace) || (t3 == SeparatorComment))
            {
               EraseLine(b2);
               EraseLine(b1);
               b1 = b3;
               b2 = NextBegin(b1);
               if(b2 == string::npos) return 0;
               continue;
            }
         }
         else if(t3 == SeparatorComment)
         {
            if(t1 == OpenBrace)
            {
               EraseLine(b3);
               EraseLine(b2);
               b2 = NextBegin(b1);
               if(b2 == string::npos) return 0;
               continue;
            }
         }
      }

      b1 = b2;
      b2 = b3;
   }

   return 0;
}

//------------------------------------------------------------------------------

word Editor::EraseExplicitTag(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseExplicitTag");

   auto ctor = log.item_->GetPos();
   if(ctor == string::npos) return NotFound(expl, "Constructor");
   auto exp = Rfind(ctor, EXPLICIT_STR);
   if(exp == string::npos) return NotFound(expl, EXPLICIT_STR, true);
   Erase(exp, strlen(EXPLICIT_STR) + 1);
   return Changed(exp, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseForward(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseForward");

   //  Erasing the forward declaration may leave an empty enclosing
   //  namespace that should be deleted.
   //
   string code;
   auto pos = CutCode(log.item_, expl, code);
   if(!expl.empty()) return -1;
   Changed();
   return EraseEmptyNamespace(pos);
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

   auto curr = CurrBegin(pos);
   if(curr == string::npos) return false;
   auto next = NextBegin(curr);
   if(next == string::npos) return false;

   //  Check that the lines can be merged.
   //
   auto type = GetLineType(curr);
   if(!LineTypeAttr::Attrs[type].isMergeable) return false;
   type = GetLineType(next);
   if(!LineTypeAttr::Attrs[type].isMergeable) return false;
   auto size = LineMergeLength
      (GetCode(curr), 0, LineSize(curr) - 1,
       GetCode(next), 0, LineSize(next) - 1);
   if(size > file_->LineLengthMax()) return false;

   //  Merge the lines after replacing or erasing CURR's endline.
   //
   auto code1 = source_.substr(curr, LineSize(curr) - 1);
   auto code2 = source_.substr(next, LineSize(next));
   auto start = LineFindFirst(next);

   if(InsertSpaceOnMerge(code1, code2, start - next))
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

word Editor::EraseLineBreak(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseLineBreak(log)");

   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound(expl, "Position for line break");
   auto merged = EraseLineBreak(begin);
   if(!merged) return Report(expl, "Line break was not removed.");
   return Changed(begin, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseMutableTag(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseMutableTag");

   //  Find the line on which the data's type appears, and erase the
   //  "mutable" before that type.
   //
   auto type = log.item_->GetTypeSpec()->GetPos();
   if(type == string::npos) return NotFound(expl, "Data type");
   auto tag = Rfind(type, MUTABLE_STR);
   if(tag == string::npos) return NotFound(expl, MUTABLE_STR, true);
   Erase(tag, strlen(MUTABLE_STR) + 1);
   return Changed(tag, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseNoexceptTag(const Function* func, string& expl)
{
   Debug::ft("Editor.EraseNoexceptTag");

   //  Look for "noexcept" just before the end of the function's signature.
   //
   auto endsig = FindSigEnd(func);
   if(endsig == string::npos) return NotFound(expl, "Signature end");
   endsig = Rfind(endsig, NOEXCEPT_STR);
   if(endsig == string::npos) return NotFound(expl, NOEXCEPT_STR, true);
   size_t space = (IsFirstNonBlank(endsig) ? 0 : 1);
   Erase(endsig - space, strlen(NOEXCEPT_STR) + space);
   return Changed(endsig, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseOffsetBlankLines()
{
   Debug::ft("Editor.EraseOffsetBlankLines");

   for(size_t begin2 = 0; begin2 != string::npos; begin2 = NextBegin(begin2))
   {
      switch(GetLineType(begin2))
      {
      case AccessControl:
      case OpenBrace:
         {
            auto begin3 = NextBegin(begin2);
            if(begin3 == string::npos) return 0;
            auto type3 = GetLineType(begin3);
            if(LineTypeAttr::Attrs[type3].isBlank)
            {
               EraseLine(begin3);
            }
            break;
         }
         //  [[fallthrough]]
      case CloseBrace:
      case CloseBraceSemicolon:
         {
            auto begin1 = PrevBegin(begin2);
            auto type1 = GetLineType(begin1);
            if(LineTypeAttr::Attrs[type1].isBlank)
            {
               EraseLine(begin1);
            }
         }
      }
   }

   return 0;
}

//------------------------------------------------------------------------------

word Editor::EraseOverrideTag(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseOverrideTag");

   //  Look for "override" just before the end of the function's signature.
   //
   auto endsig = FindSigEnd(log);
   if(endsig == string::npos) return NotFound(expl, "Signature end");
   endsig = Rfind(endsig, OVERRIDE_STR);
   if(endsig == string::npos) return NotFound(expl, OVERRIDE_STR, true);
   size_t space = (IsFirstNonBlank(endsig) ? 0 : 1);
   Erase(endsig - space, strlen(OVERRIDE_STR) + space);
   return Changed(endsig, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseParameter(const Function* func, word offset, string& expl)
{
   Debug::ft("Editor.EraseParameter");

   //  Erase the argument at OFFSET in this function definition or declaration.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::EraseSemicolon(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseSemicolon");

   //  The parser logs a redundant semicolon that follows the closing '}'
   //  of a function definition or namespace enclosure.
   //
   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound(expl, "Position of semicolon");
   auto semi = FindFirstOf(begin, ";");
   if(semi == string::npos) return NotFound(expl, "Semicolon");
   auto brace = RfindNonBlank(semi - 1);
   if(brace == string::npos) return NotFound(expl, "Right brace");
   if(source_[brace] != '}') return NotFound(expl, "Right brace");
   Erase(semi, 1);
   return Changed(semi, expl);
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
      while(IsBlank(source_[pos]) && (pos >= begin)) --pos;

      if(pos < end - 1)
      {
         Erase(pos + 1, end - pos - 1);
         Changed();
      }
   }

   return 0;
}

//------------------------------------------------------------------------------

word Editor::EraseVirtualTag(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseVirtualTag");

   //  Look for "virtual" just before the function's return type.
   //
   auto type = log.item_->GetTypeSpec()->GetPos();
   if(type == string::npos) return NotFound(expl, "Function type");
   auto virt = source_.rfind(VIRTUAL_STR, type);
   if(virt == string::npos) return NotFound(expl, VIRTUAL_STR, true);
   Erase(virt, strlen(VIRTUAL_STR) + 1);
   return Changed(type, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseVoidArgument(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseVoidArgument");

   //  The function might return "void", so the second occurrence of "void"
   //  could be the argument.  Erase it, leaving only the parentheses.
   //
   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound(expl, "Position of void argument");

   for(auto arg = FindWord(begin, VOID_STR); arg != string::npos;
      arg = FindWord(arg + 1, VOID_STR))
   {
      auto lpar = RfindNonBlank(arg - 1);
      if(lpar == string::npos) continue;
      if(source_[lpar] != '(') continue;
      auto rpar = FindNonBlank(arg + strlen(VOID_STR));
      if(rpar == string::npos) break;
      if(source_[rpar] != ')') continue;
      if(OnSameLine(arg, lpar) && OnSameLine(arg, rpar))
      {
         Erase(lpar + 1, rpar - lpar - 1);
         return Changed(arg, expl);
      }
      Erase(arg, strlen(VOID_STR));
      return Changed(arg, expl);
   }

   return NotFound(expl, VOID_STR, true);
}

//------------------------------------------------------------------------------

size_t Editor::FindAndCutInclude(size_t pos, const string& incl)
{
   Debug::ft("Editor.FindAndCutInclude");

   for(NO_OP; pos != string::npos; pos = NextBegin(pos))
   {
      if(GetCode(pos) == incl)
      {
         return EraseLine(pos);
      }
   }

   return string::npos;
}

//------------------------------------------------------------------------------

size_t Editor::FindArgsEnd(const Function* func)
{
   Debug::ft("Editor.FindArgsEnd");

   auto name = func->GetPos();
   if(name == string::npos) return string::npos;
   auto lpar = FindFirstOf(name, "(");
   if(lpar == string::npos) return string::npos;
   auto rpar = FindClosing('(', ')', lpar + 1);
   return rpar;
}

//------------------------------------------------------------------------------

size_t Editor::FindCutBegin(const CxxNamed* item) const
{
   Debug::ft("Editor.FindCutBegin");

   //  Unless the line contains multiple items, cut starting at its beginning.
   //  If there are multiple items, cut after the last delimiter before ITEM.
   //  A scope resolution operator does not qualify as a delimiter.
   //
   auto targ = item->GetPos();
   size_t pos = CurrBegin(targ);

   for(size_t next = pos; NO_OP; ++next)
   {
      next = source_.find_first_of(",(:;{}", next);
      if(next >= targ) break;
      if(CodeMatches(next, SCOPE_STR))
         ++next;
      else
         pos = next + 1;
   }

   return pos;
}

//------------------------------------------------------------------------------

size_t Editor::FindFuncDeclLoc
   (const Class* cls, const string& name, FuncDeclAttrs& attrs)
{
   Debug::ft("Editor.FindFuncDeclLoc");

   //  This currently assumes that the function to be added is an override.
   //  If there is no function with the same access control, add the access
   //  control and function in the usual order (public, protected, private).
   //  Otherwise, add the function after the first occurrence of its access
   //  control, alphabetically among any other overrides.
   //
   auto funcs = cls->Funcs();
   auto control = false;
   const Function* prev = nullptr;
   const Function* next = nullptr;

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      auto access = (*f)->GetAccess();

      if(access == attrs.access)
      {
         //  This function has the same access control.  If it's an override
         //  whose name follows NAME, insert the new function before it.
         //
         control = true;

         if((*f)->IsOverride())
         {
            if((*f)->Name()->compare(name) > 0)
            {
               next = f->get();
               break;
            }
         }
      }
      else if(access < attrs.access)
      {
         //  This function has a more restricted access control, so insert
         //  the new function before it.
         //
         next = f->get();
         break;
      }

      //  If we get here, the function will be inserted somewhere after the
      //  current one.
      //
      prev = f->get();
   }

   //  If another function with the desired access control exists, nullify
   //  this function's access control so that it won't be added redundantly.
   //
   if(control) attrs.access = Cxx::Access_N;

   //  We now know the functions between which the new function should be
   //  inserted (PREV and NEXT), so find its precise insertion location
   //  and attributes.
   //
   return UpdateFuncDeclLoc(prev, next, attrs);
}

//------------------------------------------------------------------------------

size_t Editor::FindFuncDefnLoc(const CodeFile* file,
   const Class* cls, const string& name, string& expl, FuncDefnAttrs& attrs)
{
   Debug::ft("Editor.FindFuncDefnLoc");

   //  Look at all the functions that are defined in this file and that belong
   //  to CLS.  Add the new function after the constructor, destructor, and any
   //  function whose name precedes the new function alphabetically, and add the
   //  new function before any function whose name follows the new function.
   //
   auto funcs = file->Funcs();
   const Function* prev = nullptr;
   const Function* next = nullptr;
   auto reached = false;
   auto special = true;

   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      //  Ignore instantiated functions.
      //
      if((*f)->IsInTemplateInstance()) continue;

      //  If this function is in another class, then insert the new function
      //  o immediately before it, if the new function's class has been reached
      //  o somewhere after it, if the new function's class has not been reached
      //
      if((*f)->GetClass() != cls)
      {
         if(reached)
         {
            next = (*f)->GetDefn();
            break;
         }

         prev = (*f)->GetDefn();
         continue;
      }

      reached = true;
      auto type = (*f)->FuncType();

      if((type == FuncCtor) || (type == FuncDtor))
      {
         prev = (*f)->GetDefn();
         continue;
      }

      auto currName = (*f)->Name();
      auto sort = currName->compare(name);

      if(sort > 0)
      {
         next = (*f)->GetDefn();
         break;
      }
      else if(sort < 0)
      {
         if(special ||
            (prev == nullptr) || (currName->compare(*prev->Name()) > 0))
         {
            prev = (*f)->GetDefn();
         }
      }
      else
      {
         Report(expl, "A definition for this function already exists.");
         return string::npos;
      }

      special = false;
   }

   //  We now know the functions between which the new function should be
   //  inserted (PREV and NEXT), so find its precise insertion location
   //  and attributes.
   //
   return UpdateFuncDefnLoc(prev, next, attrs);
}

//------------------------------------------------------------------------------

CodeWarning* Editor::FindLog
   (const CodeWarning& log, const CxxNamed* item, word offset)
{
   Debug::ft("Editor.FindLog");

   for(auto w = warnings_.begin(); w != warnings_.end(); ++w)
   {
      if(((*w)->warning_ == log.warning_) && ((*w)->item_ == item) &&
         ((*w)->offset_ == offset))
      {
         return *w;
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

size_t Editor::FindSigEnd(const CodeWarning& log)
{
   Debug::ft("Editor.FindSigEnd(log)");

   if(log.item_ == nullptr) return string::npos;
   if(log.item_->Type() != Cxx::Function) return string::npos;
   return FindSigEnd(static_cast< const Function* >(log.item_));
}

//------------------------------------------------------------------------------

size_t Editor::FindSigEnd(const Function* func)
{
   Debug::ft("Editor.FindSigEnd(func)");

   //  Look for the first semicolon or left brace after the function's name.
   //
   return FindFirstOf(func->GetPos(), ";{");
}

//------------------------------------------------------------------------------

size_t Editor::FindSpecialFuncLoc(const CodeWarning& log, FuncDeclAttrs& attrs)
{
   Debug::ft("Editor.FindSpecialFuncLoc");

   //  Find special member functions defined in the class that needs
   //  to have another one added.  The insertion point will be either
   //  right after the class declaration or one of these functions.
   //  The order is constructor, destructor, copy constructor, and
   //  finally copy operator.
   //
   auto where = log.item_->GetPos();
   if(where == string::npos) return string::npos;
   auto cls = static_cast< const Class* >(log.item_);
   auto ctors = cls->FindCtors();
   auto ctor = ctors.back();
   auto dtor = cls->FindFuncByRole(PureDtor, false);
   auto copy = cls->FindFuncByRole(CopyCtor, false);

   const Function* func = nullptr;  // insert right after class declaration

   switch(log.warning_)
   {
   case DefaultConstructor:
      break;

   case DefaultDestructor:
      if(ctors.front() != nullptr) func = ctor;
      break;

   case DefaultCopyConstructor:
   case RuleOf3CopyOperNoCtor:
      if(dtor != nullptr) func = dtor;
      else if(ctors.front() != nullptr) func = ctor;
      break;

   case DefaultCopyOperator:
   case RuleOf3CopyCtorNoOper:
      if(copy != nullptr) func = copy;
      else if(dtor != nullptr) func = dtor;
      else if(ctor != nullptr) func = ctor;
      break;

   default:
      return string::npos;
   };

   if(func == nullptr)
   {
      //  Insert the new function at the beginning of the class.
      //  Skip over, or insert, a "public" access control.
      //
      where = FindFirstOf(where, "{");
      where = NextBegin(where);

      if(LineFind(where, PUBLIC_STR) == string::npos)
      {
         string control(PUBLIC_STR);
         control.push_back(':');
         InsertLine(where, control);
         where = NextBegin(where);
      }

      //  If a comment will follow the new function, add a comment
      //  for it as well.
      //
      auto type = GetLineType(where);
      auto& line = LineTypeAttr::Attrs[type];
      attrs.comment = (!line.isCode && (type != BlankLine));
   }
   else
   {
      //  Insert the new function after FUNC.  If a comment precedes
      //  FUNC, also add one for the new function.
      //
      where = LineAfterFunc(func);
      auto floc = func->GetPos();
      auto type = GetLineType(PrevBegin(floc));
      auto& line = LineTypeAttr::Attrs[type];
      attrs.comment = (!line.isCode && (type != BlankLine));
   }

   //  Decide where to insert a blank line to offset the new function.
   //  LineAfterFunc often returns a blank line, so advance to the next
   //  one.  We want to return a non-blank line so that its indentation
   //  can determine the indentation for the new code.
   //
   if(IsBlankLine(where))
   {
      where = NextBegin(where);
   }

   //  Where to insert a blank line depends on whether the new function
   //  will appear first, last, or somewhere in the middle of its class.
   //
   auto prevType = GetLineType(PrevBegin(where));
   auto nextType = GetLineType(where);

   if(prevType == OpenBrace)
      attrs.blank = (nextType == CloseBraceSemicolon ? BlankNone : BlankAfter);
   else
      attrs.blank = (nextType == CloseBraceSemicolon ? BlankBefore: BlankAfter);

   //  If the new function will not be commented, don't set it off with
   //  a blank unless one precedes or follows its insertion point.
   //
   if(!attrs.comment && (prevType != BlankLine))
   {
      if((nextType == CloseBraceSemicolon) ||
         (GetLineType(NextBegin(where)) != BlankLine))
      {
         attrs.blank = BlankNone;
      }
   }

   return where;
}

//------------------------------------------------------------------------------

CxxNamedSet Editor::FindUsingReferents(const CxxNamed* item) const
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

fixed_string FixPrompt = "  Fix?";

word Editor::Fix(CliThread& cli, const FixOptions& opts, string& expl)
{
   Debug::ft("Editor.Fix");

   //  Run through all the warnings.
   //
   word rc = 0;
   auto reply = 'y';
   auto found = false;
   auto fixed = false;
   auto first = true;
   auto exit = false;

   for(auto item = warnings_.begin(); item != warnings_.end(); ++item)
   {
      //  Skip this item if the user didn't include its warning type.
      //
      if((opts.warning != AllWarnings) &&
         (opts.warning != (*item)->warning_)) continue;

      switch(FixStatus(**item))
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

      case NotSupported:
      default:
         //
         //  If multiple warning types are being fixed, try the next one.  If
         //  only one warning type was selected, there will be nothing to fix,
         //  so return a value that will terminate the >fix command.
         //
         if(opts.warning == AllWarnings) continue;
         *cli.obuf << "Fixing this warning is not supported." << CRLF;
         return -2;
      }

      //  This item is eligible for fixing.  Display it.
      //
      if(DisplayLog(cli, **item, first))
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
            reply = cli.CharPrompt(FixPrompt, YNSQChars, YNSQHelp);
         }

         switch(reply)
         {
         case 'y':
         {
            auto logs = (*item)->LogsToFix(expl);

            if(!expl.empty())
            {
               *cli.obuf << spaces(2) << expl << CRLF;
               expl.clear();
            }

            for(auto log = logs.begin(); log != logs.end(); ++log)
            {
               auto& editor = (*log)->File()->GetEditor();
               rc = editor.FixLog(cli, **log, expl);
               ReportFix(cli, rc, expl);
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
            return Report(expl, "Internal error: unknown response.", -6);
         }
      }

      cli.Flush();
      if(!opts.prompt) ThisThread::Pause(Duration(20, mSECS));
      if(exit || (rc < 0)) break;
   }

   if(found)
   {
      if(exit || (rc < 0))
         *cli.obuf << spaces(2) << "Remaining warnings skipped." << CRLF;
      else
         *cli.obuf << spaces(2) << "End of warnings." << CRLF;
   }
   else if(fixed)
   {
      *cli.obuf << spaces(2) << "Selected warning(s) in ";
      *cli.obuf << file_->Name() << " previously fixed." << CRLF;
   }
   else
   {
      if(!opts.multiple)
      {
         *cli.obuf << "No warnings that can be fixed were found." << CRLF;
      }
   }

   //  A result of -1 or greater indicates that the next file can still be
   //  processed, so return a lower value if the user wants to quit or if
   //  an error occurred when writing a file.  On a quit, a "committed"
   //  message has been displayed for the last file written, and it will
   //  be displayed again (because of rc < 0) if not cleared.
   //
   if(!Commit(cli, expl))
   {
      rc = -6;
   }
   else if((reply == 'q') && (rc >= -1))
   {
      expl.clear();
      rc = -2;
   }

   return rc;
}

//------------------------------------------------------------------------------

word Editor::FixFunction
   (const Function* func, const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.FixFunction");

   switch(log.warning_)
   {
   case ArgumentUnused:
      return EraseParameter(func, log.offset_, expl);
   case FunctionUnused:
      return EraseCode(func, expl);
   case VirtualAndPublic:
      return SplitVirtualFunction(func, expl);
   case VirtualDefaultArgument:
      return EraseDefault(func, log.offset_, expl);
   case ArgumentCouldBeConstRef:
      return TagAsConstReference(func, log.offset_, expl);
   case ArgumentCouldBeConst:
      return TagAsConstArgument(func, log.offset_, expl);
   case FunctionCouldBeConst:
      return TagAsConstFunction(func, expl);;
   case FunctionCouldBeStatic:
      return TagAsStaticFunction(func, expl);
   case FunctionCouldBeFree:
      return ChangeFunctionToFree(func, expl);
   case FunctionCouldBeDefaulted:
      return TagAsDefaulted(func, expl);
   case CouldBeNoexcept:
      return TagAsNoexcept(func, expl);
   case ShouldNotBeNoexcept:
      return EraseNoexceptTag(func, expl);
   case FunctionCouldBeMember:
      return ChangeFunctionToMember(func, log.offset_, expl);
   }

   return Report(expl, "Internal error: unexpected function warning.", -1);
}

//------------------------------------------------------------------------------

word Editor::FixFunctions(CliThread& cli, const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.FixFunctions");

   if(log.item_->Type() != Cxx::Function)
   {
      return Report(expl, "Internal error: warning is not for a function.", -1);
   }

   //  Create a list of all the function declarations and definitions that
   //  are associated with the log.
   //
   auto func = static_cast< const Function* >(log.item_);
   std::vector< const Function* > funcs;
   GetOverrides(func, funcs);

   for(auto f = funcs.cbegin(); f != funcs.cend(); ++f)
   {
      auto rc = -1;
      auto file = (*f)->GetFile();
      auto& editor = file->GetEditor();
      editor.FixFunction(*f, log, expl);

      auto fn = file->Name();

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

      ReportFix(cli, rc, expl);
   }

   return 1;
}

//------------------------------------------------------------------------------

word Editor::FixInvoker
   (const Function* func, const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.FixInvoker");

   switch(log.warning_)
   {
   case ArgumentUnused:
      return EraseArgument(func, log.offset_, expl);
   case VirtualDefaultArgument:
      return InsertArgument(func, log.offset_, expl);
   case FunctionCouldBeFree:
      return ChangeInvokerToFree(func, expl);
   case FunctionCouldBeMember:
      return ChangeInvokerToMember(func, log.offset_, expl);
   }

   return Report(expl, "Internal error: unexpected invoker warning.", -1);
}

//------------------------------------------------------------------------------

word Editor::FixInvokers(CliThread& cli, const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.FixInvokers");

   //  Use FixFunctions to modify all of the function signatures, and then
   //  use the cross-reference to find and modify all of the invocations.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::FixLog(CliThread& cli, CodeWarning& log, string& expl)
{
   Debug::ft("Editor.FixLog");

   switch(log.status)
   {
   case NotSupported:
      return Report(expl, "Fixing this warning is not supported.", 0);
   case Fixed:
   case Pending:
      return Report(expl, "This warning has already been fixed.", 0);
   }

   auto rc = FixWarning(cli, log, expl);
   if(rc == 0) log.status = Pending;
   return (rc == -1 ? 0 : rc);
}

//------------------------------------------------------------------------------

WarningStatus Editor::FixStatus(const CodeWarning& log) const
{
   Debug::ft("Editor.FixStatus");

   if((log.warning_ == IncludeNotSorted) ||
      (log.warning_ == IncludeFollowsCode))
   {
      //  If there are multiple warnings for unsorted or embedded #include
      //  directives, they all get fixed when the first one gets fixed.
      //
      if(sorted_ && (log.status == NotFixed)) return Fixed;
   }

   return log.status;
}

//------------------------------------------------------------------------------

word Editor::FixWarning(CliThread& cli, const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.FixWarning");

   switch(log.warning_)
   {
   case UseOfNull:
      return ReplaceNull(log, expl);
   case PtrTagDetached:
      return AdjustTags(log, expl);
   case RefTagDetached:
      return AdjustTags(log, expl);
   case RedundantSemicolon:
      return EraseSemicolon(log, expl);
   case RedundantConst:
      return EraseConst(log, expl);
   case DefineNotAtFileScope:
      return MoveDefine(log, expl);
   case IncludeFollowsCode:
      return SortIncludes(expl);
   case IncludeGuardMissing:
      return InsertIncludeGuard(log, expl);
   case IncludeNotSorted:
      return SortIncludes(expl);
   case IncludeDuplicated:
      return EraseCode(log.item_, expl);
   case IncludeAdd:
      return InsertInclude(log, expl);
   case IncludeRemove:
      return EraseCode(log.item_, expl);
   case RemoveOverrideTag:
      return EraseOverrideTag(log, expl);
   case UsingInHeader:
      return ReplaceUsing(log, expl);
   case UsingDuplicated:
      return EraseCode(log.item_, expl);
   case UsingAdd:
      return InsertUsing(log, expl);
   case UsingRemove:
      return EraseCode(log.item_, expl);
   case ForwardAdd:
      return InsertForward(log, expl);
   case ForwardRemove:
      return EraseForward(log, expl);
   case ArgumentUnused:
      return FixInvokers(cli, log, expl);
   case ClassUnused:
      return EraseClass(log, expl);
   case DataUnused:
      return EraseData(cli, log, expl);
   case EnumUnused:
      return EraseCode(log.item_, expl);
   case EnumeratorUnused:
      return EraseCode(log.item_, expl);
   case FriendUnused:
      return EraseCode(log.item_, expl);
   case FunctionUnused:
      return FixFunctions(cli, log, expl);
   case TypedefUnused:
      return EraseCode(log.item_, expl);
   case ForwardUnresolved:
      return EraseForward(log, expl);
   case FriendUnresolved:
      return EraseCode(log.item_, expl);
   case FriendAsForward:
      return InsertForward(log, expl);
   case HidesInheritedName:
      return ReplaceName(log, expl);
   case ClassCouldBeNamespace:
      return ChangeClassToNamespace(log, expl);
   case ClassCouldBeStruct:
      return ChangeClassToStruct(log, expl);
   case StructCouldBeClass:
      return ChangeStructToClass(log, expl);
   case RedundantAccessControl:
      return EraseAccessControl(log, expl);
   case ItemCouldBePrivate:
      return ChangeAccess(log, Cxx::Private, expl);
   case ItemCouldBeProtected:
      return ChangeAccess(log, Cxx::Protected, expl);
   case AnonymousEnum:
      return InsertEnumName(log, expl);
   case DataUninitialized:
      return InsertDataInit(log, expl);
   case DataInitOnly:
      return EraseData(cli, log, expl);
   case DataWriteOnly:
      return EraseData(cli, log, expl);
   case DataCouldBeConst:
      return TagAsConstData(log, expl);
   case DataCouldBeConstPtr:
      return TagAsConstPointer(log, expl);
   case DataNeedNotBeMutable:
      return EraseMutableTag(log, expl);
   case DefaultPODConstructor:
      return InsertPODCtor(log, expl);
   case DefaultConstructor:
      return InsertDefaultFunction(log, expl);
   case DefaultCopyConstructor:
      return InsertDefaultFunction(log, expl);
   case DefaultCopyOperator:
      return InsertDefaultFunction(log, expl);
   case NonExplicitConstructor:
      return TagAsExplicit(log, expl);
   case MemberInitMissing:
      return InsertMemberInit(log, expl);
   case MemberInitNotSorted:
      return MoveMemberInit(log, expl);
   case DefaultDestructor:
      return InsertDefaultFunction(log, expl);
   case VirtualDestructor:
      return ChangeAccess(log, Cxx::Public, expl);
   case NonVirtualDestructor:
      return TagAsVirtual(log, expl);
   case RuleOf3CopyCtorNoOper:
      return InsertDefaultFunction(log, expl);
   case RuleOf3CopyOperNoCtor:
      return InsertDefaultFunction(log, expl);
   case FunctionNotDefined:
      return EraseCode(log.item_, expl);
   case PureVirtualNotDefined:
      return InsertPureVirtual(log, expl);
   case VirtualAndPublic:
      return FixFunctions(cli, log, expl);
   case FunctionNotOverridden:
      return EraseVirtualTag(log, expl);
   case RemoveVirtualTag:
      return EraseVirtualTag(log, expl);
   case OverrideTagMissing:
      return TagAsOverride(log, expl);
   case VoidAsArgument:
      return EraseVoidArgument(log, expl);
   case AnonymousArgument:
      return AlignArgumentNames(log, expl);
   case DefinitionRenamesArgument:
      return AlignArgumentNames(log, expl);
   case OverrideRenamesArgument:
      return AlignArgumentNames(log, expl);
   case VirtualDefaultArgument:
      return FixInvokers(cli, log, expl);
   case ArgumentCouldBeConstRef:
      return FixFunctions(cli, log, expl);
   case ArgumentCouldBeConst:
      return FixFunctions(cli, log, expl);
   case FunctionCouldBeConst:
      return FixFunctions(cli, log, expl);
   case FunctionCouldBeStatic:
      return FixFunctions(cli, log, expl);
   case FunctionCouldBeFree:
      return FixInvokers(cli, log, expl);
   case StaticFunctionViaMember:
      return ChangeOperator(log, expl);
   case UseOfTab:
      return ConvertTabsToBlanks();
   case Indentation:
      return AdjustLineIndentation(log, expl);
   case TrailingSpace:
      return EraseTrailingBlanks();
   case AdjacentSpaces:
      return EraseAdjacentSpaces(log, expl);
   case AddBlankLine:
      return InsertBlankLine(log, expl);
   case RemoveBlankLine:
      return EraseBlankLine(log, expl);
   case LineLength:
      return InsertLineBreak(log, expl);
   case FunctionNotSorted:
      return MoveFunction(log, expl);
   case HeadingNotStandard:
      return ReplaceHeading(log, expl);
   case IncludeGuardMisnamed:
      return RenameIncludeGuard(log, expl);
   case DebugFtNotInvoked:
      return InsertDebugFtCall(cli, log, expl);
   case DebugFtNameMismatch:
      return ChangeDebugFtName(cli, log, expl);
   case DebugFtNameDuplicated:
      return ChangeDebugFtName(cli, log, expl);
   case DisplayNotOverridden:
      return InsertDisplay(cli, log, expl);
   case PatchNotOverridden:
      return InsertPatch(cli, log, expl);
   case FunctionCouldBeDefaulted:
      return FixFunctions(cli, log, expl);
   case InitCouldUseConstructor:
      return InitByCtorCall(log, expl);
   case CouldBeNoexcept:
      return FixFunctions(cli, log, expl);
   case ShouldNotBeNoexcept:
      return FixFunctions(cli, log, expl);
   case UseOfSlashAsterisk:
      return ReplaceSlashAsterisk(log, expl);
   case RemoveLineBreak:
      return EraseLineBreak(log, expl);
   case CopyCtorConstructsBase:
      return InsertCopyCtorCall(log, expl);
   case FunctionCouldBeMember:
      return FixInvokers(cli, log, expl);
   case ExplicitConstructor:
      return EraseExplicitTag(log, expl);
   case BitwiseOperatorOnBoolean:
      return ChangeOperator(log, expl);
   case DebugFtCanBeLiteral:
      return InlineDebugFtName(log, expl);
   }

   return Report(expl, "Fixing this warning is not supported.", 0);
}

//------------------------------------------------------------------------------

word Editor::Format(string& expl)
{
   Debug::ft("Editor.Format");

   EraseTrailingBlanks();
   EraseBlankLinePairs();
   EraseEmptySeparators();
   EraseOffsetBlankLines();
   ConvertTabsToBlanks();
   return Write(expl);
}

//------------------------------------------------------------------------------

size_t Editor::IncludesBegin() const
{
   Debug::ft("Editor.IncludesBegin");

   for(size_t pos = 0; pos != string::npos; pos = NextBegin(pos))
   {
      if(IsDirective(pos, HASH_INCLUDE_STR)) return pos;
   }

   return string::npos;
}

//------------------------------------------------------------------------------

size_t Editor::IncludesEnd() const
{
   Debug::ft("Editor.IncludesEnd");

   for(auto pos = IncludesBegin(); pos != string::npos; pos = NextBegin(pos))
   {
      if(IsDirective(pos, HASH_INCLUDE_STR)) continue;
      if(NoCodeFollows(pos)) continue;

      //  We found something else.  Back up to the last #include and return
      //  the line that follows it.
      //
      pos = PrevBegin(pos);
      while(NoCodeFollows(pos)) pos = PrevBegin(pos);
      return NextBegin(pos);
   }

   return string::npos;
}

//------------------------------------------------------------------------------

size_t Editor::Indent(size_t pos)
{
   Debug::ft("Editor.Indent");

   auto code = GetCode(pos);
   if(code.empty() || (code.front() == CRLF)) return pos;

   auto info = const_cast< const Editor& >(*this).GetLineInfo(pos);
   int depth = 0;

   if(info->depth != SIZE_MAX)
   {
      depth = info->depth;

      //  If the line is executable code, indent it one level more.  This
      //  avoids indenting comments, braces, and access controls, which
      //  are not executable.
      //
      auto type = GetLineType(info->begin);
      if(info->cont && LineTypeAttr::Attrs[type].isExecutable) ++depth;
   }

   auto first = code.find_first_not_of(WhitespaceChars);

   if(first == string::npos)
   {
      EraseLine(info->begin);
      Changed();
      return pos;
   }

   auto indent = depth * file_->IndentSize();

   if(first != indent)
   {
      Erase(info->begin, first);
      Insert(info->begin, spaces(indent));
      Changed();
   }

   return pos;
}

//------------------------------------------------------------------------------

word Editor::InitByCtorCall(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InitByCtorCall");

   //  Change ["const"] <type> <name> = <class> "(" [<args>] ");"
   //      to ["const"] <class> <name> "(" [<args>] ");"
   //
   //  Start by erasing ["const"] <type>, leaving a space before <name>.
   //
   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound(expl, "Initialization statement");
   auto first = LineFindFirst(begin);
   if(first == string::npos) return NotFound(expl, "Start of code");
   auto name = FindWord(first, *log.item_->Name());
   if(name == string::npos) return NotFound(expl, "Variable name");
   Erase(first, name - first - 1);

   //  Erase <class>.
   //
   auto eq = FindFirstOf(first, "=");
   if(eq == string::npos) return NotFound(expl, "Assignment operator");
   auto cbegin = FindNonBlank(eq + 1);
   if(cbegin == string::npos) return NotFound(expl, "Start of class name");
   auto lpar = FindFirstOf(eq, "(");
   if(lpar == string::npos) return NotFound(expl, "Left parenthesis");
   auto cend = RfindNonBlank(lpar - 1);
   if(cend == string::npos) return NotFound(expl, "End of class name");
   auto cname = source_.substr(cbegin, cend - cbegin + 1);
   Erase(cbegin, cend - cbegin + 1);

   //  Paste <class> before <name> and make it const if necessary.
   //
   Paste(first, cname, cbegin);
   if(log.item_->IsConst()) first = Insert(first, "const ");

   //  Remove the '=' and the spaces around it.
   //
   eq = FindFirstOf(first, "=");
   if(eq == string::npos) return NotFound(expl, "Assignment operator");
   auto left = RfindNonBlank(eq - 1);
   auto right = FindNonBlank(eq + 1);
   Erase(left + 1, right - left - 1);

   //  If the arguments were on a separate line, it may be possible to remove
   //  the line break.
   //
   if(!OnSameLine(begin, lpar)) EraseLineBreak(begin);
   return Changed(begin, expl);
}

//------------------------------------------------------------------------------

word Editor::InlineDebugFtName(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InlineDebugFtName");

   //  Find the fn_name data, in-line its string literal in the Debug::ft
   //  call, and then erase it.
   //
   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound(expl, "Position of Debug::ft");
   auto lpar = source_.find('(', begin);
   if(lpar == string::npos) return NotFound(expl, "Left parenthesis");
   auto rpar = source_.find(')', lpar);
   if(rpar == string::npos) return NotFound(expl, "Right parenthesis");
   auto data = static_cast< const Data* >(log.item_);
   if(data == nullptr) return NotFound(expl, "fn_name declaration");

   string fname;
   if(!data->GetStrValue(fname)) return NotFound(expl, "fn_name definition");
   auto dpos = data->GetPos();
   if(dpos == string::npos) return NotFound(expl, "fn_name in source");
   auto split = (LineFind(dpos, ";") == string::npos);
   auto next = EraseLine(dpos);
   if(split) EraseLine(next);
   auto literal = QUOTE + fname + QUOTE;
   Replace(lpar + 1, rpar - lpar - 1, literal);
   return Changed(begin, expl);
}

//------------------------------------------------------------------------------

size_t Editor::Insert(size_t pos, const string& code)
{
   Debug::ft("Editor.Insert");

   source_.insert(pos, code);
   Update();
   file_->UpdatePos(Inserted, pos, code.size());
   UpdateWarnings(Inserted, pos, code.size());
   Changed();
   return pos;
}

//------------------------------------------------------------------------------

word Editor::InsertArgument(const Function* func, word offset, string& expl)
{
   Debug::ft("Editor.InsertArgument");

   //  Change all invocations of FUNC so that any which use the default
   //  value for this argument pass the default value explicitly.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::InsertBlankLine(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertBlankLine");

   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound(expl, "Position for blank line");
   InsertLine(begin, EMPTY_STR);
   return 0;
}

//------------------------------------------------------------------------------

word Editor::InsertCopyCtorCall(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertCopyCtorCall");

   //  Have this copy constructor invoke its base class copy constructor.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::InsertDataInit(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertDataInit");

   //  Initialize this data item to its default value.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::InsertDebugFtCall
   (CliThread& cli, const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertDebugFtCall");

   auto name = log.item_->GetPos();
   if(name == string::npos) return NotFound(expl, "Function name");
   auto left = FindFirstOf(name, "{");
   if(left == string::npos) return NotFound(expl, "Left brace");

   //  Get the start of the name for an fn_name declaration and the inline
   //  string literal for the Debug::ft invocation.  Set EXTRA if anything
   //  follows the left brace on the same line.
   //
   string flit, fvar;
   DebugFtNames(static_cast< const Function* >(log.item_), flit, fvar);
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
   size_t begin, right;
   auto func = static_cast< const Function* >(log.item_);
   func->GetRange(begin, right);

   for(auto pos = left; (pos < right) && arg.empty(); pos = NextBegin(pos))
   {
      auto start = LineFind(pos, fvar);

      if(start != string::npos)
      {
         auto end = source_.find_first_not_of(ValidNextChars, start);
         if(end == string::npos) return NotFound(expl, "End of fn_name");
         arg = source_.substr(start, end - start);
      }
   }

   if(arg.empty())
   {
      //  No fn_name was defined, so use FLIT.  If another function already
      //  uses that name, prompt the user for a suffix.
      //
      if(!EnsureUniqueDebugFtName(cli, flit, arg))
         return Report(expl, FixSkipped);
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
   return Changed(below, expl);
}

//------------------------------------------------------------------------------

word Editor::InsertDefaultFunction(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertDefaultFunction");

   //  If this log is not one associated with the class itself, nothing
   //  needs to be done.
   //
   if(log.offset_ != 0)
   {
      return Report(expl, "This warning is informational and cannot be fixed.");
   }

   //  If a substitute file defines the class, the log cannot be fixed.
   //
   if(file_->IsSubsFile())
   {
      return Report(expl, "This cannot be fixed: it is in an external class.");
   }

   FuncDeclAttrs attrs(Cxx::Public);
   auto curr = FindSpecialFuncLoc(log, attrs);
   if(curr == string::npos) return NotFound(expl, "Missing function's class");

   //  Indent the code to match that at the insertion point unless the
   //  insertion point is a right brace.
   //
   string prefix;
   auto indent = LineFindFirst(curr);
   if(indent == string::npos)
      prefix = spaces(file_->IndentSize());
   else if(source_[indent] == '}')
      prefix = spaces(indent + file_->IndentSize());
   else
      prefix = spaces(indent);

   string code(prefix);
   auto className = *log.item_->Name();

   switch(log.warning_)
   {
   case DefaultConstructor:
      code += className + "() = default";
      break;

   case RuleOf3CopyOperNoCtor:
      if(log.offset_ != 0)
      {
         return Report
            (expl,"This can only be fixed if the copy operator is trivial.");
      }
      //  [[fallthrough]]
   case DefaultCopyConstructor:
      code += className;
      code += "(const " + className + "& that) = default";
      break;

   case RuleOf3CopyCtorNoOper:
      if(log.offset_ != 0)
      {
         return Report(expl,
            "This can only be fixed if the copy constructor is trivial.");
      }
      //  [[fallthrough]]
   case DefaultCopyOperator:
      code += className + "& " + "operator=";
      code += "(const " + className + "& that) = default";
      break;

   case DefaultDestructor:
      code = '~' + className + "() = default";
      break;

   default:
      return Report(expl,
         "This warning is not for an undefined special member function.");
   }

   code.push_back(';');

   if(attrs.blank == BlankAfter)
   {
      curr = InsertLine(curr, EMPTY_STR);
   }

   auto func = InsertLine(curr, code);

   if(attrs.comment)
   {
      code = prefix + COMMENT_STR;
      curr = InsertLine(func, code);

      code = prefix + COMMENT_STR + spaces(2);

      switch(log.warning_)
      {
      case DefaultConstructor:
         code += "Constructor.";
         break;
      case DefaultCopyConstructor:
      case RuleOf3CopyOperNoCtor:
         code += "Copy constructor.";
         break;
      case DefaultCopyOperator:
      case RuleOf3CopyCtorNoOper:
         code += "Copy operator.";
         break;
      case DefaultDestructor:
         code += "Destructor.";
         break;
      }

      curr = InsertLine(curr, code);
   }

   if(attrs.blank == BlankBefore)
   {
      curr = InsertLine(curr, EMPTY_STR);
   }

   return Changed(func, expl);
}

//------------------------------------------------------------------------------

word Editor::InsertDisplay(CliThread& cli, const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertDisplay");

   //  Declare an override and put "To be implemented" in the definition, with
   //  an invocation of the base class's Display function.  A more ambitious
   //  implementation would display members or invoke *their* Display functions.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::InsertEnumName(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertEnumName");

   //  Prompt for the enum's name and insert it.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::InsertForward(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertForward(log)");

   //  LOGS provides the forward's namespace and any template parameters.
   //
   string forward = spaces(file_->IndentSize()) + log.info_ + ';';
   auto srPos = forward.find(SCOPE_STR);
   if(srPos == string::npos) return NotFound(expl, "Forward's namespace.");

   //  Extract the namespace.
   //
   auto areaPos = forward.find("class ");
   if(areaPos == string::npos) areaPos = forward.find("struct ");
   if(areaPos == string::npos) areaPos = forward.find("union ");
   if(areaPos == string::npos) return NotFound(expl, "Forward's area type");

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
      if(CodeMatches(pos, NAMESPACE_STR))
      {
         //  If this namespace matches NSPACE, add the declaration to it.
         //  If this namespace's name is alphabetically after NSPACE, add
         //  the declaration before it, along with its namespace.
         //
         auto comp = source_.compare(pos, nspace.size(), nspace);
         if(comp == 0) return InsertForward(pos, forward, expl);
         if(comp > 0) return InsertNamespaceForward(pos, nspace, forward, expl);
      }
      else if(CodeMatches(pos, USING_STR)|| (pos == begin))
      {
         //  We have now passed any existing forward declarations, so add
         //  the new declaration here, along with its namespace.
         //
         return InsertNamespaceForward(pos, nspace, forward, expl);
      }
   }

   return Report(expl, "Failed to insert forward declaration.");
}

//------------------------------------------------------------------------------

word Editor::InsertForward(size_t pos, const string& forward, string& expl)
{
   Debug::ft("Editor.InsertForward(iter)");

   //  POS references a namespace that matches the one for a new forward
   //  declaration.  Insert the new declaration alphabetically within the
   //  declarations that already appear in this namespace.  Note that it
   //  may already have been inserted while fixing another warning.
   //
   for(pos = NextBegin(pos); pos != string::npos; pos = NextBegin(pos))
   {
      auto first = LineFindFirst(pos);
      if(source_[first] == '{') continue;

      auto comp = source_.compare(first, forward.size(), forward);
      if(comp == 0) return Report(expl, "Previously inserted.");

      if((comp > 0) || (source_[first] == '}'))
      {
         InsertLine(pos, forward);
         return Changed(pos, expl);
      }
   }

   return Report(expl, "Failed to insert forward declaration.");
}

//------------------------------------------------------------------------------

word Editor::InsertInclude(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertInclude(log)");

   //  Create the new #include directive and add it to the list.
   //  Its file name appears in log.info_.
   //
   string include = HASH_INCLUDE_STR;
   include.push_back(SPACE);
   include += log.info_;
   return InsertInclude(include, expl);
}

//------------------------------------------------------------------------------

word Editor::InsertInclude(string& include, string& expl)
{
   Debug::ft("Editor.InsertInclude(string)");

   //  Start by mangling the new #include and all existing ones, which
   //  makes it easier to insert the new one in the correct position.
   //
   if(MangleInclude(include, expl) != 0) return 0;
   MangleIncludes();

   //  Insert the new #include in its sort order.
   //
   auto end = IncludesEnd();

   for(auto pos = IncludesBegin(); pos != end; pos = NextBegin(pos))
   {
      if(NoCodeFollows(pos)) continue;

      if(!IncludesAreSorted(GetCode(pos), include))
      {
         InsertLine(pos, include);
         return Changed(pos, expl);
      }
   }

   //  Add the new #include to the end of the list.  If there is no
   //  blank line at the end of the list, add one.
   //
   auto pos = end;

   if(!IsBlankLine(end))
   {
      pos = InsertLine(end, EMPTY_STR);
   }

   InsertLine(pos, include);
   return Changed(pos, expl);
}

//------------------------------------------------------------------------------

word Editor::InsertIncludeGuard(const CodeWarning& log, string& expl)
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
   pos = InsertLine(pos, EMPTY_STR);
   pos = InsertLine(pos, code);
   code = "#ifndef " + guardName;
   InsertLine(pos, code);
   code = string(HASH_ENDIF_STR) + CRLF_STR;
   Insert(source_.size(), code);
   return Changed(pos, expl);
}

//------------------------------------------------------------------------------

size_t Editor::InsertLine(size_t pos, const string& code)
{
   Debug::ft("Editor.InsertLine");

   if(pos >= source_.size()) return string::npos;
   auto copy = code;
   if(copy.empty() || (copy.back() != CRLF)) copy.push_back(CRLF);
   return Insert(pos, copy);
}

//------------------------------------------------------------------------------

size_t Editor::InsertLineBreak(size_t pos)
{
   Debug::ft("Editor.InsertLineBreak(pos)");

   auto begin = CurrBegin(pos);
   if(begin == string::npos) return string::npos;
   auto end = CurrEnd(pos);
   if((pos == begin) || (pos == end)) return string::npos;
   if(IsBlankLine(pos)) return string::npos;
   Insert(pos, CRLF_STR);
   Indent(NextBegin(pos));
   return pos;
}

//------------------------------------------------------------------------------

word Editor::InsertLineBreak(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertLineBreak(log)");

   //  Consider parentheses, lexical level, binary operators...
   //
   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound(expl, "Position for line break");

   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::InsertMemberInit(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertMemberInit");

   //  Initialize the member to its default value.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::InsertNamespaceForward
   (size_t pos, const string& nspace, const string& forward, string& expl)
{
   Debug::ft("Editor.InsertNamespaceForward");

   //  Insert a new forward declaration, along with an enclosing namespace,
   //  at POS.  Offset it with blank lines.
   //
   InsertLine(pos, EMPTY_STR);
   InsertLine(pos, "}");
   auto forw = InsertLine(pos, forward);
   InsertLine(pos, "{");
   InsertLine(pos, nspace);
   InsertLine(pos, EMPTY_STR);
   return Changed(forw, expl);
}

//------------------------------------------------------------------------------

word Editor::InsertPatch(CliThread& cli, const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertPatch");

   //  Extract the name of the function to insert.  (It will be "Patch",
   //  but this code might eventually be generalized for other functions.)
   //
   auto cls = static_cast< const Class* >(log.item_);
   auto name = log.GetNewFuncName(expl);
   if(name.empty()) return -1;

   //  Find out where the function's declaration should be inserted and which
   //  file should contain its definition.  The definition is in another file,
   //  so access that file's editor and find out where the function should be
   //  defined in that file.
   //
   FuncDeclAttrs decl(Cxx::Public);
   auto pos1 = FindFuncDeclLoc(cls, name, decl);
   if(pos1 == string::npos) return -1;

   auto file = FindFuncDefnFile(cli, cls, name);
   if(file == nullptr) return -1;

   auto& editor = file->GetEditor();

   FuncDefnAttrs defn;
   auto pos2 = editor.FindFuncDefnLoc(file, cls, name, expl, defn);
   if(pos2 == string::npos) return -1;

   //  Insert the function's declaration and definition.
   //
   InsertPatchDecl(pos1, decl);
   editor.InsertPatchDefn(pos2, cls, defn);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string PatchComment = "Overridden for patching.";
fixed_string PatchReturn = "void";
fixed_string PatchSignature = "Patch(sel_t selector, void* arguments)";
fixed_string PatchInvocation = "Patch(selector, arguments)";

void Editor::InsertPatchDecl(const size_t& pos, const FuncDeclAttrs& attrs)
{
   Debug::ft("Editor.InsertPatchDecl");

   //  See if a blank line should be inserted below the function.  Then
   //  insert its definition and comment, and see if a blank line and/or
   //  access control keyword should be inserted above the function.
   //
   if(attrs.blank == BlankAfter) InsertLine(pos, EMPTY_STR);

   string code = PatchReturn;
   code.push_back(SPACE);
   code.append(PatchSignature);
   code.push_back(SPACE);
   code.append(OVERRIDE_STR);
   code.push_back(';');
   InsertLine(pos, strCode(code, 1));

   if(attrs.comment)
   {
      InsertLine(pos, strComment(EMPTY_STR, 1));
      Insert(pos, strComment(PatchComment, 1));
   }

   if(attrs.access != Cxx::Access_N)
   {
      std::ostringstream access;
      access << attrs.access << ':';
      InsertLine(pos, access.str());
   }
   else if(attrs.blank == BlankBefore)
   {
      InsertLine(pos, EMPTY_STR);
   }
}

//------------------------------------------------------------------------------

void Editor::InsertPatchDefn
   (const size_t& pos, const Class* cls, const FuncDefnAttrs& attrs)
{
   Debug::ft("Editor.InsertPatchDefn");

   //  See whether to insert a blank and rule below the function.
   //
   if(attrs.blank == BlankAfter)
   {
      InsertLine(pos, EMPTY_STR);

      if(attrs.rule)
      {
         InsertLine(pos, strRule('-'));
         InsertLine(pos, EMPTY_STR);
      }
   }

   //  Add the closing brace, the call to the base class Patch function,
   //  the opening brace, and the function signature.
   //
   InsertLine(pos, "}");

   auto base = cls->BaseClass();
   auto code = *base->Name();
   code.append(SCOPE_STR);
   code.append(PatchInvocation);
   code.push_back(';');
   InsertLine(pos, strCode(code, 1));

   InsertLine(pos, "{");

   code = PatchReturn;
   code.push_back(SPACE);
   code.append(*cls->Name());
   code.append(SCOPE_STR);
   code.append(PatchSignature);
   InsertLine(pos, code);

   //  See whether to insert a blank and rule above the function.
   //
   if(attrs.blank == BlankBefore)
   {
      InsertLine(pos, EMPTY_STR);

      if(attrs.rule)
      {
         InsertLine(pos, strRule('-'));
         InsertLine(pos, EMPTY_STR);
      }
   }
}

//------------------------------------------------------------------------------

word Editor::InsertPODCtor(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertPODCtor");

   //  Declare and define a constructor that initializes POD members to
   //  their default values.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

size_t Editor::InsertPrefix(size_t pos, const string& prefix)
{
   Debug::ft("Editor.InsertPrefix");

   auto first = LineFindFirst(pos);
   if(first == string::npos) return string::npos;

   if(pos + prefix.size() < first)
   {
      Replace(pos, prefix.size(), prefix);
   }
   else
   {
      Insert(pos, SPACE_STR);
      Insert(pos, prefix);
   }

   Changed();
   return pos;
}

//------------------------------------------------------------------------------

word Editor::InsertPureVirtual(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertPureVirtual");

   //  Insert a definition that invokes Debug::SwLog with strOver(this).
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::InsertUsing(const CodeWarning& log, string& expl)
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
      if(CodeMatches(pos, USING_STR))
      {
         //  If this using statement is alphabetically after STATEMENT,
         //  add the new statement before it.
         //
         usings = true;

         if(source_.compare(pos, statement.size(), statement) > 0)
         {
            InsertLine(pos, statement);
            return Changed(pos, expl);
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
         return Changed(pos + 1, expl);
      }
   }

   return Report(expl, "Failed to insert using statement.");
}

//------------------------------------------------------------------------------

size_t Editor::IntroStart(size_t pos, bool funcName) const
{
   Debug::ft("Editor.IntroStart");

   auto start = pos;
   auto found = false;

   for(auto curr = PrevBegin(pos); curr != string::npos; curr = PrevBegin(curr))
   {
      auto type = GetLineType(curr);

      switch(type)
      {
      case EmptyComment:
      case TextComment:
      case TaggedComment:
         //
         //  These are attached to the code that follows, so include them.
         //
         start = curr;
         break;

      case BlankLine:
         //
         //  This can be included if it precedes or follows an fn_name.
         //
         if(!funcName) return start;
         if(found) start = curr;
         break;

      case FunctionName:
         if(!funcName) return start;
         found = true;
         start = curr;

      default:
         return start;
      }
   }

   return pos;
}

//------------------------------------------------------------------------------

bool Editor::IsDirective(size_t pos, fixed_string hash) const
{
   return (source_.compare(pos, strlen(hash), hash) == 0);
}

//------------------------------------------------------------------------------

size_t Editor::LineAfterFunc(const Function* func) const
{
   Debug::ft("Editor.LineAfterFunc");

   size_t begin;
   size_t end;
   func->GetRange(begin, end);
   return NextBegin(end);
}

//------------------------------------------------------------------------------

word Editor::MangleInclude(string& include, string& expl) const
{
   Debug::ft("Editor.MangleInclude");

   //  INCLUDE is enclosed in angle brackets or quotes.  To simplify
   //  sorting, the following substitutions are made:
   //  o group 1: enclosed in [ ]
   //  o group 2: enclosed in ' '
   //  o group 3: enclosed in ( )
   //  o group 4: enclosed in ` `
   //  This is fixed when the #include is written out.
   //
   if(include.find(HASH_INCLUDE_STR) != 0)
      return Report(expl, "#include not at front of directive.", -1);
   auto first = include.find_first_of(FrontChars);
   if(first == string::npos)
      return Report(expl, "Failed to extract file name from #include.", -1);
   auto last = include.find_first_of(BackChars, first + 1);
   if(last == string::npos)
      return Report(expl, "Failed to extract file name from #include.", -1);
   auto name = include.substr(first + 1, last - first - 1);
   auto group = file_->CalcGroup(name);
   if(group == 0) return Report(expl, "#include specified unknown file.", -1);
   include[first] = FrontChars[group - 1];
   include[last] = BackChars[group - 1];
   return 0;
}

//------------------------------------------------------------------------------

void Editor::MangleIncludes()
{
   Debug::ft("Editor.MangleIncludes");

   string expl;

   for(size_t pos = 0; pos != string::npos; pos = NextBegin(pos))
   {
      if(IsDirective(pos, HASH_INCLUDE_STR))
      {
         auto incl = GetCode(pos);
         MangleInclude(incl, expl);
         source_.erase(pos, incl.size());
         source_.insert(pos, incl);
      }
   }
}

//------------------------------------------------------------------------------

word Editor::MoveDefine(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.MoveDefine");

   //  Move the #define directly after the #include directives.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::MoveFunction(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.MoveFunction");

   //  Move the function's definition to the correct location.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::MoveMemberInit(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.MoveMemberInit");

   //  Move the member to the correct location in the initialization list.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

fn_name Editor_Paste = "Editor.Paste";

size_t Editor::Paste(size_t pos, const std::string& code, size_t from)
{
   Debug::ft(Editor_Paste);

   if(from != lastCut_)
   {
      Debug::SwLog(Editor_Paste, "Illegal Paste operation: " + code, from);
      return string::npos;
   }

   source_.insert(pos, code);
   lastCut_ = string::npos;
   Update();
   file_->UpdatePos(Pasted, pos, code.size(), from);
   UpdateWarnings(Pasted, pos, code.size(), from);
   Changed();
   return pos;
}

//------------------------------------------------------------------------------

size_t Editor::PrologEnd() const
{
   Debug::ft("Editor.PrologEnd");

   for(size_t pos = 0; pos != string::npos; pos = NextBegin(pos))
   {
      if(LineTypeAttr::Attrs[GetLineType(pos)].isCode) return pos;
   }

   return string::npos;
}

//------------------------------------------------------------------------------

void Editor::QualifyReferent(const CxxNamed* item, const CxxNamed* ref)
{
   Debug::ft("Editor.QualifyReferent");

   //  Within ITEM, prefix NS wherever SYMBOL appears as an identifier.
   //
   const Namespace* ns = ref->GetSpace();
   auto symbol = ref->Name();

   switch(ref->Type())
   {
   case Cxx::Namespace:
      ns = static_cast< const Namespace* >(ref)->OuterSpace();
      break;
   case Cxx::Class:
      if(ref->IsInTemplateInstance())
      {
         auto tmplt = ref->GetTemplate();
         ns = tmplt->GetSpace();
         symbol = tmplt->Name();
      }
   }

   auto qual = ns->ScopedName(false) + SCOPE_STR;
   size_t pos, end;
   item->GetRange(pos, end);
   Reposition(pos);
   string name;

   while(FindIdentifier(name, false))
   {
      if(Curr() > end) return;

      if(name == *symbol)
      {
         //  Qualify NAME with NS if it is not already qualified.
         //
         pos = Curr();

         if(source_.rfind(SCOPE_STR, pos) != (pos - strlen(SCOPE_STR)))
         {
            Insert(pos, qual);
            Changed();
            Advance(qual.size());
         }
      }

      //  Look for the next name after the current one.
      //
      Advance(name.size());
   }
}

//------------------------------------------------------------------------------

void Editor::QualifyUsings(const CxxNamed* item)
{
   Debug::ft("Editor.QualifyUsings");

   auto refs = FindUsingReferents(item);

   for(auto r = refs.cbegin(); r != refs.cend(); ++r)
   {
      QualifyReferent(item, *r);
   }
}

//------------------------------------------------------------------------------

word Editor::RenameIncludeGuard(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.RenameIncludeGuard");

   //  This warning is logged against the #define.
   //
   auto def = CurrBegin(log.Pos());
   if(def == string::npos) return NotFound(expl, "Position of #define");
   if(!IsDirective(def, HASH_DEFINE_STR))
      return NotFound(expl, HASH_DEFINE_STR);
   auto guard = log.File()->MakeGuardName();
   def += strlen(HASH_DEFINE_STR) + 1;
   auto end = CurrEnd(def) - 1;
   Erase(def, end - def + 1);
   Insert(def, guard);
   auto ifn = Rfind(def, HASH_IFNDEF_STR);
   if(ifn == string::npos) return NotFound(expl, HASH_IFNDEF_STR);
   ifn += strlen(HASH_IFNDEF_STR) + 1;
   end = CurrEnd(ifn) - 1;
   Erase(ifn, end - ifn + 1);
   Insert(ifn, guard);
   return Changed(NextBegin(ifn), expl);
}

//------------------------------------------------------------------------------

size_t Editor::Replace(size_t pos, size_t count, const std::string& code)
{
   Debug::ft("Editor.Replace");

   Erase(pos, count);
   Insert(pos, code);
   return pos;
}

//------------------------------------------------------------------------------

word Editor::ReplaceHeading(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ReplaceHeading");

   //  Remove the existing header and replace it with the standard one,
   //  inserting the file name where appropriate.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::ReplaceName(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ReplaceName");

   //  Prompt for a new name that will replace the existing one.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::ReplaceNull(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ReplaceNull");

   //  If there are multiple occurrences on the same line, each one will
   //  cause a log, so just fix the first one.
   //
   auto begin = CurrBegin(log.Pos());
   if(begin == string::npos) return NotFound(expl, "Position of NULL");
   auto null = FindWord(begin, NULL_STR);
   if(null == string::npos) return NotFound(expl, NULL_STR, true);
   Replace(null, strlen(NULL_STR), NULLPTR_STR);
   return Changed(null, expl);
}

//------------------------------------------------------------------------------

word Editor::ReplaceSlashAsterisk(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ReplaceSlashAsterisk");

   auto pos0 = CurrBegin(log.Pos());
   if(pos0 == string::npos) return NotFound(expl, "Position of /*");
   auto pos1 = source_.find(COMMENT_BEGIN_STR, pos0);
   if(pos1 == string::npos) return NotFound(expl, COMMENT_BEGIN_STR);
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
      return Report(expl, "Unchanged: code follows /*...*/");
   }
   else if(pos3 == string::npos)  // [1]
   {
      Erase(pos1, strlen(COMMENT_BEGIN_STR));
      Changed();
   }
   else if((pos2 == string::npos) && (pos3 != string::npos))  // [2]
   {
      source_.replace(pos1, strlen(COMMENT_BEGIN_STR), COMMENT_STR);
      Changed();
   }
   else  // [3]
   {
      Erase(pos2, strlen(COMMENT_END_STR));
      source_.replace(pos1, strlen(COMMENT_BEGIN_STR), COMMENT_STR);
      return Changed(pos1, expl);
   }

   //  Now continue with subsequent lines:
   //  o pos1: original start of /*
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
         if(pos3 > pos1) pos3 = pos1;
         InsertPrefix(pos3, COMMENT_STR);
         Changed();
      }
      else if((pos3 == string::npos) && (pos4 == string::npos))  // [2]
      {
         Erase(pos2, strlen(COMMENT_END_STR));
         return Changed(PrevBegin(pos2), expl);
      }
      else if((pos3 == string::npos) && (pos4 != string::npos))  // [3]
      {
         Erase(pos2, strlen(COMMENT_END_STR));
         Changed();
         pos2 = InsertLineBreak(pos2);
         return Changed(pos2, expl);
      }
      else if((pos3 != string::npos) && (pos4 == string::npos))  // [4]
      {
         Erase(pos2, strlen(COMMENT_END_STR));
         if(pos3 > pos1) pos3 = pos1;
         InsertPrefix(pos3, COMMENT_STR);
         return Changed(pos3, expl);
      }
      else  // [5]
      {
         Erase(pos2, strlen(COMMENT_END_STR));
         InsertLineBreak(pos2);
         if(pos3 > pos1) pos3 = pos1;
         InsertPrefix(pos3, COMMENT_STR);
         return Changed(pos3, expl);
      }
   }

   return Report(expl, "Closing */ not found.  Inspect changes!", -1);
}

//------------------------------------------------------------------------------

word Editor::ReplaceUsing(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ReplaceUsing");

   //  Before removing the using statement, add type aliases to each class
   //  for symbols that appear in its definition and that were resolved by
   //  a using statement.
   //
   ResolveUsings();
   return EraseCode(log.item_, expl);
}

//------------------------------------------------------------------------------

word Editor::ResolveUsings()
{
   Debug::ft("Editor.ResolveUsings");

   //  This function only needs to be run once per file.
   //
   if(aliased_) return 0;

   //  Classes, data, functions, and typedefs can contain names resolved by
   //  using statements.  Modify each of these so that using statements can
   //  be removed.
   //
   auto classes = file_->Classes();

   for(auto c = classes->cbegin(); c != classes->cend(); ++c)
   {
      auto refs = FindUsingReferents(*c);

      for(auto r = refs.cbegin(); r != refs.cend(); ++r)
      {
         QualifyReferent(*c, *r);
      }
   }

   //  Qualify names in non-class items at file scope.
   //
   auto data = file_->Datas();
   for(auto d = data->cbegin(); d != data->cend(); ++d)
   {
      QualifyUsings(*d);
   }

   auto funcs = file_->Funcs();
   for(auto f = funcs->cbegin(); f != funcs->cend(); ++f)
   {
      QualifyUsings(*f);
   }

   auto types = file_->Types();
   for(auto t = types->cbegin(); t != types->cend(); ++t)
   {
      QualifyUsings(*t);
   }

   aliased_ = true;
   return 0;
}

//------------------------------------------------------------------------------

void Editor::Setup(CodeFile* file)
{
   Debug::ft("Editor.Setup");

   if((file_ != nullptr) || (file == nullptr)) return;

   //  Get the file's code and configure the lexer.  The code is read in
   //  again because Lexer.Preprocess has modified the original version.
   //
   file_ = file;
   file_->ReadCode(source_);
   Initialize(source_, file_);
   CalcDepths();

   //  Get the file's warnings and sort them for fixing.  The order reduces
   //  the chances of an item's position changing before it is edited.
   //
   CodeWarning::GetWarnings(file_, warnings_);
   std::sort(warnings_.begin(), warnings_.end(), CodeWarning::IsSortedToFix);
}

//------------------------------------------------------------------------------

word Editor::SortIncludes(string& expl)
{
   Debug::ft("Editor.SortIncludes");

   //  Start by mangling all #includes, which makes them easier to sort.
   //
   MangleIncludes();

   //  Copy all of the #includes into a list and sort them.  Look at the
   //  entire file in case any #include appears outside the initial list.
   //
   std::list< string > includes;

   for(auto pos = 0; pos != string::npos; pos = NextBegin(pos))
   {
      if(IsDirective(pos, HASH_INCLUDE_STR))
      {
         includes.push_back(GetCode(pos));
      }
   }

   if(includes.empty()) return NotFound(expl, HASH_INCLUDE_STR);
   includes.sort(IncludesAreSorted);

   //  Run through the #includes.  When one doesn't match the sort order,
   //  find the correct one, cut it, and paste it into the current location.
   //
   auto targ = includes.cbegin();

   for(auto pos = IncludesBegin();
      (pos != string::npos) && (targ != includes.cend());
      pos = NextBegin(pos), ++targ)
   {
      if(GetCode(pos) != *targ)
      {
         auto from = FindAndCutInclude(pos, *targ);

         if(from != string::npos)
         {
            Paste(pos, *targ, from);
         }
         else
         {
            auto err = string("Failed to find ") + *targ;
            return Report(expl, err.c_str());
         }
      }
   }

   sorted_ = true;
   Changed();
   return Report(expl, "All #includes sorted.");
}

//------------------------------------------------------------------------------

word Editor::SplitVirtualFunction(const Function* func, string& expl)
{
   Debug::ft("Editor.SplitVirtualFunction");

   //  Split this public virtual function:
   //  o Rename the function, its overrides, and its invocations within
   //    overrides to its original name + "_v".
   //  o Make its declaration protected and virtual.
   //  o Make its public declaration non-virtual, with the implementation
   //    simply invoking its renamed, protected version.
   //
   return Unimplemented(expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsConstArgument(const Function* func, word offset, string& expl)
{
   Debug::ft("Editor.TagAsConstArgument");

   //  Find the line on which the argument's type appears, and insert
   //  "const" before that type.
   //
   auto index = func->LogOffsetToArgIndex(offset);
   auto type = func->GetArgs().at(index)->GetTypeSpec()->GetPos();
   Insert(type, "const ");
   return Changed(type, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsConstData(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.TagAsConstData");

   //  Find the line on which the data's type appears, and insert
   //  "const" before that type.
   //
   auto type = log.item_->GetTypeSpec()->GetPos();
   if(type == string::npos) return NotFound(expl, "Data type");
   Insert(type, "const ");
   return Changed(type, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsConstFunction(const Function* func, string& expl)
{
   Debug::ft("Editor.TagAsConstFunction");

   //  Insert " const" after the right parenthesis at the end of the function's
   //  argument list.
   //
   auto endsig = FindArgsEnd(func);
   if(endsig == string::npos) return NotFound(expl, "End of argument list");
   Insert(endsig + 1, " const");
   return Changed(endsig, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsConstPointer(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.TagAsConstPointer");

   //  If there is more than one pointer, this applies to the last one, so
   //  back up from the data item's name.  Search for the name on this line
   //  in case other edits have altered its position.
   //
   auto data = log.item_->GetPos();
   if(data == string::npos) return NotFound(expl, "Data member");
   auto name = FindWord(data, *log.item_->Name());
   if(name == string::npos) return NotFound(expl, "Member name");
   auto ptr = Rfind(name, "*");
   if(ptr == string::npos) return NotFound(expl, "Pointer tag");
   Insert(ptr + 1, " const");
   return Changed(ptr, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsConstReference
   (const Function* func, word offset, string& expl)
{
   Debug::ft("Editor.TagAsConstReference");

   //  Find the line on which the argument's name appears.  Insert a reference
   //  tag before the argument's name and "const" before its type.  The tag is
   //  added first so that its position won't change as a result of adding the
   //  "const" earlier in the line.
   //
   auto& args = func->GetArgs();
   auto index = func->LogOffsetToArgIndex(offset);
   auto arg = args.at(index).get();
   if(arg == nullptr) return NotFound(expl, "Argument");
   auto pos = arg->GetPos();
   if(pos == string::npos) return NotFound(expl, "Argument name");
   auto prev = RfindNonBlank(pos - 1);
   Insert(prev + 1, "&");
   auto rc = TagAsConstArgument(func, offset, expl);
   if(rc != 0) return rc;
   expl.clear();
   return Changed(prev, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsDefaulted(const Function* func, string& expl)
{
   Debug::ft("Editor.TagAsDefaulted");

   //  If this is a separate definition, delete it.
   //
   if(func->GetDecl() != func)
   {
      return EraseCode(func, expl);
   }

   //  This is the function's declaration, and possibly its definition.
   //
   auto endsig = FindSigEnd(func);
   if(endsig == string::npos) return NotFound(expl, "Signature end");
   if(source_[endsig] == ';')
   {
      Insert(endsig, " = default");
   }
   else
   {
      auto right = FindFirstOf(endsig + 1, "}");
      if(right == string::npos) return NotFound(expl, "Right brace");
      Erase(endsig, right - endsig + 1);
      Insert(endsig, "= default;");
   }

   return Changed(endsig, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsExplicit(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.TagAsExplicit");

   //  A constructor can be tagged "constexpr", which the parser looks for
   //  only *after* "explicit".
   //
   auto ctor = log.item_->GetPos();
   if(ctor == string::npos) return NotFound(expl, "Constructor");
   auto prev = source_.rfind(CONSTEXPR_STR, ctor);
   if(prev != string::npos) ctor = prev;
   Insert(ctor, "explicit ");
   return Changed(ctor, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsNoexcept(const Function* func, string& expl)
{
   Debug::ft("Editor.TagAsNoexcept");

   //  Insert "noexcept" after "const" but before "override" or "final".
   //  Start by finding the end of the function's argument list.
   //
   auto pos = func->GetPos();
   if(pos == string::npos) return NotFound(expl, "Function name");
   auto rpar = FindArgsEnd(func);
   if(rpar == string::npos) return NotFound(expl, "End of argument list");

   //  If there is a "const" after the right parenthesis, insert "noexcept"
   //  after it, else insert "noexcept" after the right parenthesis.
   //
   auto cons = FindNonBlank(rpar + 1);
   if(CodeMatches(cons, CONST_STR))
   {
      Insert(cons + strlen(CONST_STR), " noexcept");
      return Changed(cons, expl);
   }

   Insert(rpar + 1, " noexcept");
   return Changed(rpar, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsOverride(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.TagAsOverride");

   //  Insert "override" after the last non-blank character at the end
   //  of the function's signature.
   //
   auto endsig = FindSigEnd(log);
   if(endsig == string::npos) return NotFound(expl, "Signature end");
   endsig = RfindNonBlank(endsig - 1);
   Insert(endsig + 1, " override");
   return Changed(endsig, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsStaticFunction(const Function* func, string& expl)
{
   Debug::ft("Editor.TagAsStaticFunction");

   //  Start with the function's return type in case it's on the line above
   //  the function's name.  Then find the end of the argument list.
   //
   auto type = func->GetTypeSpec()->GetPos();
   if(type == string::npos) return NotFound(expl, "Function type");
   auto rpar = FindArgsEnd(func);
   if(rpar == string::npos) return NotFound(expl, "End of argument list");

   //  Only a function's declaration, not its definition, is tagged "static".
   //  Start by removing any "virtual" tag.  Then put "static" after "extern"
   //  and/or "inline".
   //
   if(func->GetDecl() == func)
   {
      auto front = source_.rfind(VIRTUAL_STR, type);
      if(front != string::npos) Erase(front, strlen(VIRTUAL_STR) + 1);
      front = source_.rfind(INLINE_STR, type);
      if(front != string::npos) type = front;
      front = source_.rfind(EXTERN_STR, type);
      if(front != string::npos) type = front;
      Insert(type, "static ");
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

         if(OnSameLine(rpar, tag))
         {
            if(IsBlank(source_[tag - 1])) Erase(tag - 1, 1);
         }
         else
         {
            if(IsBlank(source_[tag])) Erase(tag, 1);
         }
      }
   }

   return Changed(type, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsVirtual(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.TagAsVirtual");

   //  Make this destructor virtual.
   //
   auto pos = Insert(log.item_->GetPos(), "virtual ");
   return Changed(pos, expl);
}

//------------------------------------------------------------------------------

void Editor::UpdateFuncDeclAttrs(const Function* func, FuncDeclAttrs& attrs) const
{
   Debug::ft("Editor.UpdateFuncDeclAttrs");

   if(func == nullptr) return;

   //  If FUNC is commented, a blank line should also precede or follow it.
   //
   size_t begin, end;
   func->GetRange(begin, end);
   auto type = GetLineType(PrevBegin(begin));
   auto& line = LineTypeAttr::Attrs[type];

   if(!line.isCode && (type != BlankLine))
   {
      attrs.comment = true;
      attrs.blank = BlankBefore;
      return;
   }

   //  FUNC isn't commented, so see if a blank line precedes or follows it.
   //
   if(type == BlankLine)
   {
      attrs.blank = BlankBefore;
      return;
   }

   if(GetLineType(NextBegin(end)) == BlankLine)
   {
      attrs.blank = BlankBefore;
   }
}

//------------------------------------------------------------------------------

fn_name Editor_UpdateFuncDeclLoc = "Editor.UpdateFuncDeclLoc";

size_t Editor::UpdateFuncDeclLoc
   (const Function* prev, const Function* next, FuncDeclAttrs& attrs) const
{
   Debug::ft(Editor_UpdateFuncDeclLoc);

   //  PREV and NEXT are the functions that precede and follow the function
   //  whose declaration is to be inserted.
   //
   UpdateFuncDeclAttrs(prev, attrs);
   UpdateFuncDeclAttrs(next, attrs);

   if(prev != nullptr)
   {
      //  Insert the function after PREV.
      //
      return LineAfterFunc(prev);
   }

   if(next == nullptr)
   {
      Debug::SwLog(Editor_UpdateFuncDeclLoc, "prev and next are nullptr", 0);
      return string::npos;
   }

   //  Insert the function before NEXT.  If the new function is to be offset
   //  with a blank, it will follow the new function.
   //
   if(attrs.blank != BlankNone) attrs.blank = BlankAfter;

   auto pred = PrevBegin(next->GetPos());

   while(true)
   {
      auto type = GetLineType(pred);

      if(!LineTypeAttr::Attrs[type].isCode)
      {
         if(type == BlankLine) break;

         //  This is a comment, so keep moving up to find the insertion point.
         //
         pred = PrevBegin(pred);
         continue;
      }

      //  If an access control precedes NEXT, a blank line is not required
      //  after the new function.
      //
      if(GetLineType(pred) == AccessControl)
      {
         attrs.blank = BlankNone;
         return pred;
      }

      break;
   }

   //  PRED is a blank line or code other than an access control.  Insert the
   //  new function above the line that follows PRED.
   //
   return NextBegin(pred);
}

//------------------------------------------------------------------------------

void Editor::UpdateFuncDefnAttrs(const Function* func, FuncDefnAttrs& attrs) const
{
   Debug::ft("Editor.UpdateFuncDefnAttrs");

   if(func == nullptr) return;

   //  See if FUNC is preceded or followed by a rule and/or blank line.
   //
   bool blank = false;
   size_t begin, end;
   func->GetRange(begin, end);
   auto pos = NextBegin(end);
   auto type = GetLineType(pos);

   if(type == BlankLine)
   {
      blank = true;
      type = GetLineType(NextBegin(pos));
      if(type == SeparatorComment)
      {
         attrs.rule = true;
         attrs.blank = BlankBefore;
         return;
      }
   }

   pos = begin;
   while(true)
   {
      pos = PrevBegin(pos);
      type = GetLineType(pos);

      switch(type)
      {
      case SeparatorComment:
         attrs.rule = true;
         attrs.blank = BlankBefore;
         return;

      case BlankLine:
         blank = true;
         //  [[fallthrough]]
      case FunctionName:
         continue;

      default:
         if(blank) attrs.blank = BlankBefore;
         return;
      }
   }
}

//------------------------------------------------------------------------------

size_t Editor::UpdateFuncDefnLoc
   (const Function* prev, const Function* next, FuncDefnAttrs& attrs) const
{
   Debug::ft("Editor.UpdateFuncDefnLoc");

   //  PREV and NEXT are the functions that precede and follow the function
   //  whose definition is to be inserted.
   //
   UpdateFuncDefnAttrs(prev, attrs);
   UpdateFuncDefnAttrs(next, attrs);

   if(prev != nullptr)
   {
      //  Insert the function after the one that precedes it.
      //
      return LineAfterFunc(prev);
   }

   if(next == nullptr)
   {
      //  Insert a rule above the function.  There must be something else
      //  in the file!
      //
      attrs.rule = true;
      attrs.blank = BlankBefore;

      //  Insert the function at the bottom of the file.  If it ends with
      //  two closing braces, the second one ends a namespace definition,
      //  so insert the definition above that one.
      //
      auto pos = PrevBegin(string::npos);
      auto type = GetLineType(pos);
      if(type != CloseBrace) return string::npos;
      type = GetLineType(PrevBegin(pos));
      if(type != CloseBrace) return string::npos;
      return pos;
   }

   //  Insert the function before NEXT.  If NEXT has an fn_name, insert the
   //  definition above *that*.  If the new function is to be offset with a
   //  blank and/or rule, they need to go *after* the new function.
   //
   if(attrs.blank != BlankNone) attrs.blank = BlankAfter;

   auto pred = PrevBegin(next->GetPos());

   while(true)
   {
      auto type = GetLineType(pred);

      switch(type)
      {
      case BlankLine:
         pred = PrevBegin(pred);
         continue;
      case FunctionName:
         while(GetLineType(--pred) == FunctionName);
         return ++pred;
      default:
         return pred;
      }
   }

   return pred;
}

//------------------------------------------------------------------------------

void Editor::UpdateWarnings
   (EditorAction action, size_t begin, size_t count, size_t from) const
{
   Debug::ft("Editor.UpdateWarnings");

   for(auto w = warnings_.begin(); w != warnings_.end(); ++w)
   {
      (*w)->UpdatePos(action, begin, count, from);
   }
}

//------------------------------------------------------------------------------

word Editor::Write(string& expl)
{
   Debug::ft("Editor.Write");

   std::ostringstream stream;

   //  Create a new file to hold the reformatted version.
   //
   auto path = file_->Path();
   auto temp = path + ".tmp";
   auto output = SysFile::CreateOstream(temp.c_str(), true);
   if(output == nullptr)
   {
      stream << "Failed to open output file for " << file_->Name();
      return Report(expl, stream, -7);
   }

   //  #includes for files that define base classes used in this file or declare
   //  functions defined in this file had their angle brackets or quotes mangled
   //  for sorting purposes.  Fix this.
   //
   for(size_t pos = 0; pos != string::npos; pos = NextBegin(pos))
   {
      if(IsDirective(pos, HASH_INCLUDE_STR))
      {
         auto incl = GetCode(pos);
         DemangleInclude(incl);
         source_.erase(pos, incl.size());
         source_.insert(pos, incl);
      }
   }

   *output << source_;

   //  Delete the original file and replace it with the new one.
   //
   output.reset();
   auto err = remove(path.c_str());

   if(err != 0)
   {
      stream << "Failed to remove " << file_->Name() << ": error=" << err;
      return Report(expl, stream, err);
   }

   err = rename(temp.c_str(), path.c_str());

   if(err != 0)
   {
      stream << "Failed to rename " << file_->Name() << ": error=" << err;
      return Report(expl, stream, err);
   }

   for(auto w = warnings_.begin(); w != warnings_.end(); ++w)
   {
      if((*w)->status == Pending) (*w)->status = Fixed;
   }

   stream << "..." << file_->Name() << " committed";
   return Report(expl, stream);
}
}
