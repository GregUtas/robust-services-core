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
#include "Lexer.h"
#include "Library.h"
#include "NbCliParms.h"
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
//  Returns true if the code at ITER is empty.
//
bool CodeIsEmpty(const SourceIter& iter)
{
   return (iter->code.find_first_not_of(WhitespaceChars) == string::npos);
}

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
//  If the code at ITER is an #include directive, unmangles and returns it,
//  else simply returns it without any changes.
//
string DemangleInclude(const SourceIter& iter)
{
   auto& code = iter->code;

   if(code.find(HASH_INCLUDE_STR) != 0) return code;

   auto front = code.find_first_of(FrontChars);
   if(front == string::npos) return code;
   auto back = RfindFirstNotOf(code, code.size() - 1, WhitespaceChars);

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

   return true;
}

//------------------------------------------------------------------------------

fixed_string FilePrompt = "Enter the filename in which to define";

const CodeFile* FindFuncDefnFile
   (CliThread& cli, const Class* cls, const string& name)
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

bool IncludesSorted(const SourceLine& line1, const SourceLine& line2)
{
   return IncludesAreSorted(line1.code, line2.code);
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
   return spaces(level * INDENT_SIZE) + code;
}

//------------------------------------------------------------------------------
//
//  Returns TEXT, prefixed by "//  " and indented to LEVEL standard
//  indentations.
//
string strComment(const string& text, size_t level)
{
   return spaces(level * INDENT_SIZE) + "//  " + text;
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

Editor::Editor(const CodeFile& file) :
   file_(&file),
   sorted_(false),
   aliased_(false)
{
   Debug::ft("Editor.ctor");

   //  Get the file's source code.  If this fails, there is no point in
   //  continuing.
   //
   if(!source_.Initialize(*file_)) return;

   //  Get the file's warnings and sort them for fixing.  The order reduces
   //  the chances of an item's position changing before it is edited.
   //
   CodeWarning::GetWarnings(file_, warnings_);
   std::sort(warnings_.begin(), warnings_.end(), CodeWarning::IsSortedToFix);

   //  Mangle #include directives.  This is done to assist in sorting them.
   //
   string expl;

   for(auto i = Code().begin(); i != Code().end(); ++i)
   {
      if(i->code.find(HASH_INCLUDE_STR) == 0)
      {
         MangleInclude(i->code, expl);
      }
   }

   source_.CalcDepths();
   source_.ClassifyLines();
}

//------------------------------------------------------------------------------

word Editor::AdjustLineIndentation(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.AdjustLineIndentation");

   auto s = FindLine(log.line_, expl);
   if(s == Code().end()) return 0;
   Indent(s, false);
   return Changed(s, expl);
}

//------------------------------------------------------------------------------

word Editor::AdjustTags(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.AdjustTags");

   auto tag = (log.warning_ == PtrTagDetached ? '*' : '&');
   string target = "Detached ";
   target.push_back(tag);
   target.push_back(SPACE);

   auto loc = FindPos(log.pos_);
   if(loc.pos == string::npos) NotFound(expl, target.c_str());

   //  A pointer tag should not be preceded by a space.  It either adheres to
   //  its type or "const".  The same is true for a reference tag, which can
   //  also adhere to a pointer tag.  Even if there is more than one detached
   //  pointer tag, only one log is generated, so fix them all.
   //
   auto changed = false;

   for(auto tagpos = loc.iter->code.find(tag, loc.pos); tagpos != string::npos;
       tagpos = loc.iter->code.find(tag, tagpos + 1))
   {
      if(IsBlank(loc.iter->code[tagpos - 1]))
      {
         auto prev = RfindNonBlank(loc.iter, tagpos - 1);
         auto count = tagpos - (prev.pos + 1);
         loc.iter->code.erase(prev.pos + 1, count);
         tagpos -= count;

         //  If the character after the tag is the beginning of an identifier,
         //  insert a space.
         //
         auto next = tagpos + 1;

         if((next < loc.iter->code.size()) &&
            (ValidFirstChars.find(loc.iter->code[next]) != string::npos))
         {
            loc.iter->code.insert(next, 1, SPACE);
         }

         changed = true;
      }
   }

   if(changed) return Changed(loc.iter, expl);
   return NotFound(expl, target.c_str());
}

//------------------------------------------------------------------------------

word Editor::AlignArgumentNames(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.AlignArgumentNames");

   //  This handles the following argument warnings:
   //  o AnonymousArgument: unnamed argument
   //  o DefinitionRenamesArgument: definition's name differs from declaration's
   //  o OverrideRenamesArgument: override's name differs from base's
   //
   auto func = static_cast< const Function* >(log.item_);
   const Function* decl = func->GetDecl();
   const Function* defn = func->GetDefn();
   const Function* base = (func->IsOverride() ? func->GetBase() : nullptr);
   if(decl == nullptr) return NotFound(expl, "Function's declaration");

   //  Use the argument name from the base (if any), else the declaration,
   //  else the definition.
   //
   auto index = decl->LogOffsetToArgIndex(log.offset_);
   string argName = EMPTY_STR;
   auto declName = *decl->GetArgs().at(index)->Name();
   string defnName = EMPTY_STR;
   if(defn != nullptr) defnName = *defn->GetArgs().at(index)->Name();
   if(base != nullptr) argName = *base->GetArgs().at(index)->Name();
   if(argName.empty()) argName = declName;
   if(argName.empty()) argName = defnName;
   if(argName.empty()) return NotFound(expl, "Candidate argument name");

   //  The declaration and definition are logged separately, so fix only
   //  the one that has a problem.
   //
   if(log.warning_ == AnonymousArgument)
   {
      auto arg = func->GetArgs().at(index).get();
      auto loc = FindPos(arg->GetPos());
      if(loc.pos == string::npos) return NotFound(expl, "Argument");
      loc = FindFirstOf(loc, ",)");
      if(loc.pos == string::npos) return NotFound(expl, "End of argument");
      loc.iter->code.insert(loc.pos, 1, SPACE);
      loc.iter->code.insert(loc.pos + 1, argName);
      return Changed(loc.iter, expl);
   }

   size_t first, last;
   func->GetRange(first, last);
   if(last == string::npos) return NotFound(expl, "End of function");
   auto begin = FindPos(log.item_->GetPos());
   if(begin.pos == string::npos) return NotFound(expl, "Function name");
   auto lastLine = log.file_->GetLexer().GetLineNum(last);
   auto end = FindLine(lastLine);
   if(end == Code().end()) return NotFound(expl, "Function last line");
   if(func == decl) defnName = declName;
   auto range = lastLine - log.line_ + 1;

   for(auto loc = FindWord(begin.iter, begin.pos, defnName, &range);
       loc.pos != string::npos;
       loc = FindWord(loc.iter, loc.pos + 1, defnName, &range))
   {
      loc.iter->code.erase(loc.pos, defnName.size());
      loc.iter->code.insert(loc.pos, argName);
      Changed();
   }

   return Changed(begin.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::ChangeAccess
   (const CodeWarning& log, Cxx::Access acc, string& expl)
{
   Debug::ft("Editor.ChangeAccess");

   //  Move the item and insert the access control keyword if necessary.
   //
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::ChangeClassToNamespace(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ChangeClassToNamespace");

   //  Replace "class" with "namespace" and "static" with "extern" (for data)
   //  or nothing (for functions).  Delete things that are no longer needed:
   //  base class, access controls, special member functions, and closing ';'.
   //
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::ChangeClassToStruct(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ChangeClassToStruct");

   //  Look for the class's name and then back up to "class".
   //
   auto cls = FindPos(log.item_->GetPos());
   if(cls.pos == string::npos) return NotFound(expl, "Class name");
   cls = Rfind(cls.iter, CLASS_STR, cls.pos);
   if(cls.pos == string::npos) return NotFound(expl, CLASS_STR, true);
   cls.iter->code.erase(cls.pos, strlen(CLASS_STR));
   cls.iter->code.insert(cls.pos, STRUCT_STR);

   //  If the class began with a "public:" access control, erase it.
   //
   auto left = Find(cls.iter, "{");
   if(left.pos == string::npos) return NotFound(expl, "Left brace");
   source_.Next(left);
   auto acc = FindWord(left.iter, left.pos, PUBLIC_STR);
   if(acc.pos != string::npos)
   {
      auto colon = FindNonBlank(acc.iter, acc.pos + strlen(PUBLIC_STR));
      colon.iter->code.erase(colon.pos, 1);
      acc.iter->code.erase(acc.pos, strlen(PUBLIC_STR));
      if(CodeIsEmpty(acc.iter)) Code().erase(acc.iter);
   }
   return Changed(cls.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::Changed()
{
   Debug::ft("Editor.Changed");

   Editors_.insert(this);
   return 0;
}

//------------------------------------------------------------------------------

word Editor::Changed(const SourceIter& iter, string& expl)
{
   Debug::ft("Editor.Changed(iter)");

   expl = (CodeIsEmpty(iter) ? EMPTY_STR : iter->code);
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
   auto wloc = FindPos(log.pos_);
   if(wloc.pos == string::npos) return NotFound(expl, "Warning location");
   auto cloc = Find(wloc.iter, "Debug::ft", wloc.pos);
   if(cloc.pos == string::npos) return NotFound(expl, "Debug::ft invocation");

   //  Find the location of the first fn_name definition that precedes this
   //  function.  If one is found, it belongs to a previous function if a
   //  right brace appears between it and the start of this function.
   //
   auto floc = FindPos(log.item_->GetPos());
   if(floc.pos == string::npos) return NotFound(expl, "Function name");
   auto dloc = Rfind(floc.iter, "fn_name", floc.pos);

   if(dloc.iter != Code().end())
   {
      auto valid = true;

      for(auto i = dloc.iter; valid && (i != floc.iter); ++i)
      {
         if(i->code.find('}') != string::npos) valid = false;
      }

      if(!valid) dloc = source_.End();
   }

   //  Generate the string (FLIT) and fn_name (FVAR).  If FLIT is already
   //  in use, prompt the user for a unique suffix.
   //
   string flit, fvar, fname;

   DebugFtNames(static_cast< const Function* >(log.item_), flit, fvar);
   if(!EnsureUniqueDebugFtName(cli, flit, fname))
      return Report(expl, FixSkipped);

   if(dloc.iter == Code().end())
   {
      //  An fn_name definition was not found, so the Debug::ft invocation
      //  must have used a string literal.  Replace the invocation.
      //
      auto call = DebugFtCode(fname);
      cloc.iter = Code().erase(cloc.iter);
      cloc.iter = Code().insert(cloc.iter, SourceLine(call, SIZE_MAX));
      return Changed(cloc.iter, expl);
   }

   //  The Debug::ft invocation used an fn_name.  It might be used elsewhere
   //  (e.g. for calls to Debug::SwLog), so keep its name and only replace
   //  its definition.
   //
   auto lquo = Find(dloc.iter, QUOTE_STR, dloc.pos);
   if(lquo.pos == string::npos) return NotFound(expl, "fn_name left quote");
   auto rquo = lquo.iter->code.find(QUOTE, lquo.pos + 1);
   if(rquo == string::npos) return NotFound(expl, "fn_name right quote");
   lquo.iter->code.erase(lquo.pos + 1, rquo - lquo.pos - 1);
   lquo.iter->code.insert(lquo.pos + 1, fname);

   if(lquo.iter->code.size() > file_->LineLengthMax())
   {
      auto eq = FindFirstOf(dloc, "=");
      if(eq.pos != string::npos) InsertLineBreak(eq.iter, eq.pos + 1);
   }

   return Changed(lquo.iter, expl);
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
   return Unimplemented(expl);  //*
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
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::ChangeInvokerToFree(const Function* func, string& expl)
{
   Debug::ft("Editor.ChangeInvokerToFree");

   //  Change invokers of this function to invoke it directly instead of
   //  through its class.  An invoker not in the same namespace may have
   //  to prefix the function's namespace.
   //
   return Unimplemented(expl);  //*
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
   return Unimplemented(expl);  //*
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
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::ChangeStructToClass(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ChangeStructToClass");

   //  Look for the struct's name and then back up to "struct".
   //
   auto str = FindPos(log.item_->GetPos());
   if(str.pos == string::npos) return NotFound(expl, "Struct name");
   str = Rfind(str.iter, STRUCT_STR, str.pos);
   if(str.pos == string::npos) return NotFound(expl, STRUCT_STR, true);
   str.iter->code.erase(str.pos, strlen(STRUCT_STR));
   str.iter->code.insert(str.pos, CLASS_STR);

   //  Unless the struct began with a "public:" access control, insert one.
   //
   auto left = Find(str.iter, "{");
   if(left.pos == string::npos) return NotFound(expl, "Left brace");
   source_.Next(left);
   auto acc = FindWord(left.iter, left.pos, PUBLIC_STR);
   if(acc.pos == string::npos)
   {
      string control(PUBLIC_STR);
      control.push_back(':');
      Insert(left.iter, control);
   }
   return Changed(str.iter, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_CodeBegin = "Editor.CodeBegin";

SourceIter Editor::CodeBegin()
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

   size_t first = SIZE_MAX;

   for(auto p = positions.cbegin(); p != positions.end(); ++p)
   {
      if(*p < first) first = *p;
   }

   auto s = Code().end();

   if(first != SIZE_MAX)
   {
      auto line = file_->GetLexer().GetLineNum(first);
      s = FindLine(line);
   }

   auto ns = false;

   for(--s; s != Code().begin(); --s)
   {
      auto type = GetLineType(s);

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
         //  This should be the brace for a namespace enclosure.
         //
         ns = true;
         break;

      case CodeLine:
         //
         //  If we saw an open brace, this should be a namespace enclosure.
         //  Generate a log if it's something else, otherwise continue to
         //  back up.  If a namespace wasn't expected, this is probably a
         //  forward declaration, so assume that the code starts after it.
         //
         if(ns)
         {
            if(s->code.find(NAMESPACE_STR) != string::npos) continue;
            Debug::SwLog(Editor_CodeBegin, "namespace expected", s->line + 1);
         }
         return ++s;

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
         return ++s;
      }
   }

   return s;
}

//------------------------------------------------------------------------------

const bool Editor::CodeFollowsImmediately(const SourceIter& iter)
{
   Debug::ft("Editor.CodeFollowsImmediately");

   //  Proceed from ITER, skipping blank lines and access controls.  Return
   //  false if the next thing is executable code (this excludes braces and
   //  access controls), else return false.
   //
   for(auto next = std::next(iter); next != Code().end(); ++next)
   {
      auto type = GetLineType(next);

      if(LineTypeAttr::Attrs[type].isExecutable) return true;
      if(LineTypeAttr::Attrs[type].isBlank) continue;
      if(type == AccessControl) continue;
      return false;
   }

   return false;
}

//------------------------------------------------------------------------------

word Editor::ConvertTabsToBlanks()
{
   Debug::ft("Editor.ConvertTabsToBlanks");

   auto indent = file_->IndentSize();

   //  Run through the source, looking for a line of code that contains a tab.
   //
   for(auto s = Code().begin(); s != Code().end(); ++s)
   {
      auto pos = s->code.find_first_of(TAB);

      if(pos != string::npos)
      {
         //  A tab has been found.  Copy the characters that precede it
         //  into UNTABBED.  Run through the rest of the line, copying
         //  non-tab characters to UNTABBED.  Replace each tab with the
         //  number of spaces needed to reach the next tab stop, namely
         //  the next even multiple of INDENT.  This code doesn't bother
         //  to skip over character and string literals, which should use
         //  \t or TAB or something that is actually visible.
         //
         string untabbed = s->code.substr(0, pos);

         for(NO_OP; pos < s->code.size(); ++pos)
         {
            auto c = s->code.at(pos);

            if(c != TAB)
            {
               untabbed.push_back(c);
            }
            else
            {
               size_t spaces = untabbed.size() % indent;
               if(spaces == 0) spaces = indent;

               for(NO_OP; spaces > 0; --spaces)
               {
                  untabbed.push_back(SPACE);
               }
            }
         }

         //  Replace this line of code with its tabless equivalent and
         //  mark the file as modified.
         //
         s->code = untabbed;
         Changed();
      }
   }

   return 0;
}

//------------------------------------------------------------------------------

string Editor::DebugFtCode(const string& fname) const
{
   Debug::ft("Editor.DebugFtCode(inline)");

   auto call = string(file_->IndentSize(), SPACE) + "Debug::ft(";
   call.push_back(QUOTE);
   call.append(fname);
   call.push_back(QUOTE);
   call.append(");");
   return call;
}

//------------------------------------------------------------------------------

void Editor::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Base::Display(stream, prefix, options);

   stream << prefix << "file     : " <<
      (file_ != nullptr ? file_->Name() : "no file specified") << CRLF;
   stream << CRLF;
   stream << prefix << "sorted   : " << sorted_ << CRLF;
   stream << prefix << "aliased  : " << aliased_ << CRLF;
   stream << prefix << "warnings : " << warnings_.size() << CRLF;
   stream << prefix << "source   : " << CRLF;

   auto indent = prefix + spaces(2);
   source_.Display(stream, indent, options);
}

//------------------------------------------------------------------------------

void Editor::DisplayLog(const CliThread& cli, const CodeWarning& log, bool file)
{
   Debug::ft("Editor.DisplayLog");

   if(file)
   {
      *cli.obuf << log.file_->Name() << ':' << CRLF;
   }

   //  Display LOG's details.
   //
   *cli.obuf << "  Line " << log.line_ + 1;
   if(log.offset_ > 0) *cli.obuf << '/' << log.offset_;
   *cli.obuf << ": " << Warning(log.warning_);
   if(log.HasInfoToDisplay()) *cli.obuf << ": " << log.info_;
   *cli.obuf << CRLF;

   if(log.HasCodeToDisplay())
   {
      //  Display the current version of the code associated with LOG.
      //
      auto s = FindLine(log.line_);

      if(s == Code().end())
      {
         *cli.obuf << "  Line " << log.line_ + 1 << " not found." << CRLF;
         return;
      }

      *cli.obuf << spaces(2);
      *cli.obuf << DemangleInclude(s);
   }
}

//------------------------------------------------------------------------------

word Editor::EraseAccessControl(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseAccessControl");

   //  The parser logs RedundantAccessControl at the position
   //  where it occurred; log.item_ is nullptr in this warning.
   //
   auto s = FindLine(log.line_, expl);
   if(s == Code().end()) return 0;
   size_t len = 0;

   //  Look for the access control keyword and note its length.
   //
   auto acc = FindWord(s, 0, PUBLIC_STR);

   while(true)
   {
      if(acc.pos != string::npos)
      {
         len = strlen(PUBLIC_STR);
         break;
      }

      acc = FindWord(s, 0, PROTECTED_STR);

      if(acc.pos != string::npos)
      {
         len = strlen(PROTECTED_STR);
         break;
      }

      acc = FindWord(s, 0, PRIVATE_STR);

      if(acc.pos != string::npos)
      {
         len = strlen(PRIVATE_STR);
         break;
      }

      return NotFound(expl, "Access control keyword");
   }

   //  Look for the colon that follows the keyword.
   //
   auto col = FindNonBlank(acc.iter, acc.pos + len);
   if((col.pos == string::npos) || (col.iter->code[col.pos] != ':'))
      return NotFound(expl, "Colon after access control");

   //  Erase the keyword and colon.
   //
   col.iter->code.erase(col.pos, 1);
   acc.iter->code.erase(acc.pos, len);
   return Changed(acc.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseAdjacentSpaces(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseAdjacentSpaces");

   auto s = FindLine(log.line_, expl);
   if(s == Code().end()) return 0;

   auto& code = s->code;
   auto pos = code.find_first_not_of(WhitespaceChars);
   if(pos == string::npos) return 0;

   //  If this line has a trailing comment that is aligned with one on the
   //  previous or the next line, keep the comments aligned by moving the
   //  erased spaces immediately to the left of the comment.
   //
   auto move = false;
   auto comm = code.find_first_of("//");

   if(comm != string::npos)
   {
      if(s != Code().begin())
      {
         auto prev = std::prev(s);
         move = (comm == prev->code.find_first_of("//"));
      }

      if(!move)
      {
         auto next = std::next(s);
         if(next != Code().end())
         {
            move = (comm == next->code.find_first_of("//"));
         }
      }
   }

   //  Don't erase adjacent spaces that precede a trailing comment.
   //
   auto stop = comm;

   if(stop != string::npos)
      while(IsBlank(code[stop - 1])) --stop;
   else
      stop = code.size() - 1;

   comm = stop;  // (comm - stop) will be number of erased spaces

   while(pos + 1 < stop)
   {
      if(IsBlank(code[pos]) && IsBlank(code[pos + 1]))
      {
         code.erase(pos, 1);
         --stop;
      }
      else
      {
         ++pos;
      }
   }

   if(move) code.insert(stop, string(comm - stop, SPACE));
   return Changed(s, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseArgument(const Function* func, word offset, string& expl)
{
   Debug::ft("Editor.EraseArgument");

   //  In this function invocation, erase the argument at OFFSET.
   //
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::EraseBlankLine(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseBlankLine");

   //  Remove the specified line of code.
   //
   auto blank = FindPos(log.pos_);
   if(blank.pos == string::npos) return NotFound(expl, "Blank line");
   Code().erase(blank.iter);
   return Changed();
}

//------------------------------------------------------------------------------

word Editor::EraseBlankLinePairs()
{
   Debug::ft("Editor.EraseBlankLinePairs");

   auto i1 = Code().begin();
   if(i1 == Code().end()) return 0;

   for(auto i2 = std::next(i1); i2 != Code().end(); i2 = std::next(i1))
   {
      if(CodeIsEmpty(i1) && CodeIsEmpty(i2))
      {
         i1 = Code().erase(i1);
         Changed();
      }
      else
      {
         i1 = i2;
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
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::EraseCode(size_t pos, string& expl)
{
   Debug::ft("Editor.EraseCode");

   //  Erase the line that contains POS.
   //
   auto begin = FindPos(pos);
   if(begin.pos == string::npos) return NotFound(expl, "Code to be deleted");
   Code().erase(begin.iter);
   return Changed();
}

//------------------------------------------------------------------------------

word Editor::EraseCode(size_t pos, const string& delimiters, string& expl)
{
   Debug::ft("Editor.EraseCode(delimiters)");

   //  Find the where the code to be deleted begins and ends.
   //
   auto begin = FindPos(pos);
   if(begin.pos == string::npos)
      return NotFound(expl, "Start of code to be deleted");
   auto end = FindFirstOf(begin, delimiters);
   if(end.pos == string::npos)
      return NotFound(expl, "End of code to be deleted");

   //  When a member initialization is being deleted and a left brace delimits
   //  the code, replace the previous comma with a space instead of erasing the
   //  left brace.
   //
   if(end.iter->code.at(end.pos) == '{')
   {
      source_.Prev(end);

      if(delimiters.find(',') != string::npos)
      {
         auto comma = Rfind(begin.iter, ",", begin.pos);

         if(comma.iter != Code().end())
         {
            comma.iter->code.at(comma.pos) = SPACE;
         }
      }
   }

   //  If the code to be deleted isn't immediately followed by code, delete
   //  any comment that precedes the code, since it almost certainly refers
   //  only to that code.
   //
   if(!CodeFollowsImmediately(end.iter))
   {
      begin.iter = IntroStart(begin.iter, false);
   }

   Code().erase(begin.iter, std::next(end.iter));
   return Changed();
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
   auto tag = FindPos(log.pos_);
   if(tag.pos == string::npos) return NotFound(expl, "Redundant const");

   for(tag = FindWord(tag.iter, tag.pos, CONST_STR);
       tag.pos != string::npos;
       tag = FindWord(tag.iter, tag.pos + 1, CONST_STR))
   {
      auto prev = RfindNonBlank(tag.iter, tag.pos - 1);

      //  This is the first non-blank character preceding a subsequent
      //  "const".  If it's a pointer tag, it makes the pointer const,
      //  so continue with the next "const".
      //
      switch(prev.iter->code[prev.pos])
      {
      case '*':
      case ',':
      case '(':
         continue;
      }

      //  This is the redundant const, so erase it.  Also erase a space
      //  between it and the previous non-blank character.
      //
      tag.iter->code.erase(tag.pos, strlen(CONST_STR));

      if((prev.iter == tag.iter) && (tag.pos - prev.pos > 1))
      {
         prev.iter->code.erase(prev.pos + 1, 1);
      }

      return Changed(tag.iter, expl);
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

      auto editor = file->GetEditor(expl);

      if(expl.empty())
      {
         auto delimiters = ((*r)->Type() == Cxx::MemberInit ? ",{" : ";");
         editor->EraseCode(pos, delimiters, expl);
         if(!expl.empty())
         {
            expl = "Failed to remove usage at " + file->Name() + '/';
            expl += std::to_string(file->GetLexer().GetLineNum(pos));
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
      auto editor = defn->GetFile()->GetEditor(expl);

      if(expl.empty())
      {
         editor->EraseCode(defn->GetPos(), ";", expl);
         if(!expl.empty()) expl = "Failed to remove definition";
      }

      if(!expl.empty())
      {
         *cli.obuf << spaces(2) << expl << CRLF;
         expl.clear();
      }
   }

   //  Erase the data declaration.
   //
   return EraseCode(log.pos_, ";", expl);
}

//------------------------------------------------------------------------------

word Editor::EraseDefault(const Function* func, word offset, string& expl)
{
   Debug::ft("Editor.EraseDefault");

   //  Erase this argument's default value.
   //
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::EraseEmptyNamespace(const SourceIter& iter)
{
   Debug::ft("Editor.EraseEmptyNamespace");

   //  ITER references the line that follows a forward declaration which was
   //  just deleted.  If this left an empty "namespace <ns> { }", remove it.
   //
   if(iter == Code().end()) return 0;
   if(iter->code.find('}') != 0) return 0;

   auto up1 = std::prev(iter);
   if(up1 == Code().begin()) return 0;
   auto up2 = std::prev(up1);

   if((up2->code.find(NAMESPACE_STR) == 0) && (up1->code.find('{') == 0))
   {
      Code().erase(up2, std::next(iter));
      return Changed();
   }

   return 0;
}

//------------------------------------------------------------------------------

word Editor::EraseEmptySeparators()
{
   Debug::ft("Editor.EraseEmptySeparators");

   auto i1 = Code().begin();
   if(i1 == Code().end()) return 0;
   auto i2 = std::next(i1);
   if(i2 == Code().end()) return 0;

   for(auto i3 = std::next(i2); i3 != Code().end(); i3 = std::next(i2))
   {
      auto t1 = GetLineType(i1);
      auto t2 = GetLineType(i2);
      auto t3 = GetLineType(i3);

      if(LineTypeAttr::Attrs[t2].isBlank)
      {
         if(t1 == SeparatorComment)
         {
            if((t3 == CloseBrace) || (t3 == SeparatorComment))
            {
               Code().erase(i1);
               Code().erase(i2);
               i1 = i3;
               i2 = std::next(i1);
               if(i2 == Code().end()) return 0;
               continue;
            }
         }
         else if(t3 == SeparatorComment)
         {
            if(t1 == OpenBrace)
            {
               Code().erase(i2);
               i2 = Code().erase(i3);
               if(i2 == Code().end()) return 0;
               continue;
            }
         }
      }

      i1 = i2;
      i2 = i3;
   }

   return 0;
}

//------------------------------------------------------------------------------

word Editor::EraseEnumerator(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseEnumerator");

   auto etor = static_cast< const Enumerator* >(log.item_);
   auto curr = FindPos(log.pos_);
   if(curr.pos == string::npos) return NotFound(expl, "Enumerator's line");

   //  There are various possibilities, including more than one enumerator
   //  on the same line.  Badly formatted code may not survive what follows,
   //  but anything reasonable should.  Start by erasing the enumerator.
   //
   curr = FindWord(curr.iter, 0, *etor->Name());
   if(curr.pos == string::npos) return NotFound(expl, "Enumerator");
   curr.iter->code.erase(curr.pos, etor->Name()->size());

   //  Now erase any comma that follows.
   //
   auto next = FindNonBlank(curr.iter, curr.pos);
   if(next.iter->code.at(next.pos) == ',')
   {
      next.iter->code.erase(curr.pos, next.pos - curr.pos + 1);
   }

   //  Now erase any comment that follows immediately.
   //
   auto after = curr.iter->code.find_first_not_of(WhitespaceChars, curr.pos);
   if((after != string::npos) &&
      (curr.iter->code.find(COMMENT_STR, after) == after))
   {
      curr.iter->code.erase(curr.pos);
   }

   //  If the erasures have left a blank line, delete it.
   //
   if(curr.iter->code.find_first_not_of(WhitespaceChars) == string::npos)
   {
      Code().erase(curr.iter);
   }

   //  If this was the last enumerator, replace the comma that follows the
   //  previous one with a space.  The space will preserve the alignment of
   //  any trailing comments or get erased just before the file is written.
   //  Note that the previous enumerator(s) could also have been erased, so
   //  keep looking until the previous one is found.
   //
   auto eNum = static_cast< const Enum* >(etor->AutoType());
   auto& etors = eNum->Etors();
   if(etors.back().get() != etor) return Changed();

   for(auto e = std::next(etors.rbegin()); e != etors.rend(); ++e)
   {
      etor = e->get();
      auto prev = FindPos(etor->GetPos());
      if(prev.pos == string::npos) continue;
      prev = FindWord(prev.iter, 0, *etor->Name());
      if(prev.pos == string::npos) continue;
      after = prev.iter->code.find(',', prev.pos + etor->Name()->size());
      if(after != string::npos)
      {
         prev.iter->code.replace(after, 1, 1, SPACE);
         break;
      }
   }

   return Changed();
}

//------------------------------------------------------------------------------

word Editor::EraseExplicitTag(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseExplicitTag");

   auto ctor = FindPos(log.item_->GetPos());
   if(ctor.pos == string::npos) return NotFound(expl, "Constructor");
   auto exp = ctor.iter->code.find(EXPLICIT_STR, ctor.pos);
   if(exp == string::npos) return NotFound(expl, EXPLICIT_STR, true);
   ctor.iter->code.erase(exp, strlen(EXPLICIT_STR) + 1);
   return Changed(ctor.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseForward(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseForward");

   //  Erase the line where the forward declaration appears.  This
   //  may leave an empty enclosing namespace that should be deleted.
   //
   auto forw = FindPos(log.pos_);
   if(forw.pos == string::npos)
      return NotFound(expl, "Forward declaration");
   forw.iter = Code().erase(forw.iter);
   Changed();
   return EraseEmptyNamespace(forw.iter);
}

//------------------------------------------------------------------------------

word Editor::EraseFunction(const Function* func, string& expl)
{
   Debug::ft("Editor.EraseFunction");

   //  It's easy if we're erasing a declaration that has a separate definition.
   //
   if(func->GetDefn() != func)
   {
      return EraseCode(func->GetPos(), ";", expl);
   }

   //  The function contains code, so find its first and last lines.
   //
   size_t begin, end;
   func->GetRange(begin, end);
   auto first = FindPos(begin);
   if(first.iter == Code().end())
      return NotFound(expl, "Start of declaration");
   auto last = FindPos(end);
   if(last.iter == Code().end()) return NotFound(expl, "End of declaration");

   //  Also delete any comments and fn_name definition above the function.
   //
   first.iter = IntroStart(first.iter, true);
   Code().erase(first.iter, std::next(last.iter));
   return Changed();
}

//------------------------------------------------------------------------------

bool Editor::EraseLineBreak(const SourceIter& curr)
{
   Debug::ft("Editor.EraseLineBreak(iter)");

   auto next = std::next(curr);
   if(next == Code().end()) return false;

   //  Check that the lines can be merged.
   //
   auto type = GetLineType(curr);
   if(!LineTypeAttr::Attrs[type].isMergeable) return false;
   type = GetLineType(next);
   if(!LineTypeAttr::Attrs[type].isMergeable) return false;
   auto size = LineMergeLength
      (curr->code, 0, curr->code.size() - 1,
       next->code, 0, next->code.size() - 1);
   if(size > file_->LineLengthMax()) return false;

   //  Merge the lines after discarding CURR's endline.
   //
   curr->code.pop_back();
   auto start = next->code.find_first_not_of(WhitespaceChars);
   if(InsertSpaceOnMerge(curr->code, next->code, start))
   {
      curr->code.push_back(SPACE);
   }

   next->code.erase(0, start);
   curr->code.append(next->code);
   Code().erase(next);
   return true;
}

//------------------------------------------------------------------------------

word Editor::EraseLineBreak(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseLineBreak(log)");

   auto iter = FindLine(log.line_, expl);
   if(iter == Code().end()) return 0;
   auto merged = EraseLineBreak(iter);
   if(!merged) return Report(expl, "Line break was not removed.");
   return Changed(iter, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseMutableTag(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseMutableTag");

   //  Find the line on which the data's type appears, and erase the
   //  "mutable" before that type.
   //
   auto type = FindPos(log.item_->GetTypeSpec()->GetPos());
   if(type.pos == string::npos) return NotFound(expl, "Data type");
   auto tag = Rfind(type.iter, MUTABLE_STR, type.pos);
   if(tag.pos == string::npos) return NotFound(expl, MUTABLE_STR, true);
   tag.iter->code.erase(tag.pos, strlen(MUTABLE_STR) + 1);
   return Changed(tag.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseNoexceptTag(const Function* func, string& expl)
{
   Debug::ft("Editor.EraseNoexceptTag");

   //  Look for "noexcept" just before the end of the function's signature.
   //
   auto endsig = FindSigEnd(func);
   if(endsig.pos == string::npos) return NotFound(expl, "Signature end");
   endsig = Rfind(endsig.iter, NOEXCEPT_STR, endsig.pos - 1);
   if(endsig.pos == string::npos) return NotFound(expl, NOEXCEPT_STR, true);
   size_t space = (endsig.pos == 0 ? 0 : 1);
   endsig.iter->code.erase(endsig.pos - space, strlen(NOEXCEPT_STR) + space);
   return Changed(endsig.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseOffsets()
{
   Debug::ft("Editor.EraseOffsets");

   for(auto i2 = Code().begin(); i2 != Code().end(); ++i2)
   {
      switch(GetLineType(i2))
      {
      case AccessControl:
      case OpenBrace:
         {
            auto i3 = std::next(i2);
            if(i3 == Code().end()) return 0;
            auto t3 = GetLineType(i3);
            if(LineTypeAttr::Attrs[t3].isBlank)
            {
               Code().erase(i3);
            }
            break;
         }
         //  [[fallthrough]]
      case CloseBrace:
      case CloseBraceSemicolon:
         {
            auto i1 = std::prev(i2);
            auto t1 = GetLineType(i1);
            if(LineTypeAttr::Attrs[t1].isBlank)
            {
               Code().erase(i1);
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
   if(endsig.pos == string::npos) return NotFound(expl, "Signature end");
   endsig = Rfind(endsig.iter, OVERRIDE_STR, endsig.pos - 1);
   if(endsig.pos == string::npos) return NotFound(expl, OVERRIDE_STR, true);
   size_t space = (endsig.pos == 0 ? 0 : 1);
   endsig.iter->code.erase(endsig.pos - space, strlen(OVERRIDE_STR) + space);
   return Changed(endsig.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseParameter(const Function* func, word offset, string& expl)
{
   Debug::ft("Editor.EraseParameter");

   //  Erase the parameter at OFFSET in this function definition or declaration.
   //
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::EraseSemicolon(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseSemicolon");

   //  The parser logs a redundant semicolon that follows the closing '}'
   //  of a function definition or namespace enclosure.
   //
   auto s = FindLine(log.line_, expl);
   if(s == Code().end()) return 0;
   auto semi = s->code.rfind(';');
   if(semi == string::npos) return NotFound(expl, "Semicolon");
   auto brace = RfindNonBlank(s, semi - 1);
   if(brace.pos == string::npos) return NotFound(expl, "Right brace");
   if(brace.iter->code[brace.pos] == '}')
   {
      s->code.erase(semi, 1);
      return Changed(s, expl);
   }
   return NotFound(expl, "Right brace");
}

//------------------------------------------------------------------------------

word Editor::EraseTrailingBlanks()
{
   Debug::ft("Editor.EraseTrailingBlanks");

   for(auto s = Code().begin(); s != Code().end(); ++s)
   {
      auto size = s->code.size();

      while(!s->code.empty() && IsBlank(s->code.back()))
      {
         s->code.pop_back();
      }

      s->code.push_back(CRLF);
      if(s->code.size() != size) Changed();
   }

   return 0;
}

//------------------------------------------------------------------------------

word Editor::EraseVirtualTag(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseVirtualTag");

   //  Look for "virtual" just before the function's return type.
   //
   auto type = FindPos(log.item_->GetTypeSpec()->GetPos());
   if(type.pos == string::npos) return NotFound(expl, "Function type");
   auto virt = type.iter->code.rfind(VIRTUAL_STR, type.pos);
   if(virt == string::npos) return NotFound(expl, VIRTUAL_STR, true);
   type.iter->code.erase(virt, strlen(VIRTUAL_STR) + 1);
   return Changed(type.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::EraseVoidArgument(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.EraseVoidArgument");

   //  The function might return "void", so the second occurrence of "void"
   //  could be the argument.  Erase it, leaving only the parentheses.
   //
   auto s = FindLine(log.line_, expl);
   if(s == Code().end()) return 0;

   for(auto arg = FindWord(s, 0, VOID_STR); arg.pos != string::npos;
       arg = FindWord(arg.iter, arg.pos + 1, VOID_STR))
   {
      auto lpar = RfindNonBlank(arg.iter, arg.pos - 1);
      if(lpar.pos == string::npos) continue;
      if(lpar.iter->code[lpar.pos] != '(') continue;
      auto rpar = FindNonBlank(arg.iter, arg.pos + strlen(VOID_STR));
      if(rpar.pos == string::npos) break;
      if(rpar.iter->code[rpar.pos] != ')') continue;
      if((arg.iter == lpar.iter) && (arg.iter == rpar.iter))
      {
         arg.iter->code.erase(lpar.pos + 1, rpar.pos - (lpar.pos + 1));
         return Changed(arg.iter, expl);
      }
      arg.iter->code.erase(arg.pos, strlen(VOID_STR));
      return Changed(arg.iter, expl);
   }

   return NotFound(expl, VOID_STR, true);
}

//------------------------------------------------------------------------------

SourceLoc Editor::Find(SourceIter iter, const string& str, size_t off)
{
   Debug::ft("Editor.Find");

   while(iter != Code().end())
   {
      auto pos = iter->code.find(str, off);
      if(pos != string::npos) return SourceLoc(iter, pos);
      ++iter;
      off = 0;
   }

   return SourceLoc(iter);
}

//------------------------------------------------------------------------------

SourceLoc Editor::FindArgsEnd(const Function* func)
{
   Debug::ft("Editor.FindArgsEnd");

   auto& args = func->GetArgs();
   if(args.empty())
   {
      //  Find the right parenthesis after the function's name.
      //
      auto name = FindPos(func->GetPos());
      if(name.pos == string::npos) return source_.End();
      return FindFirstOf(name, ")");
   }

   //  Find the right parenthesis after the last argument's name.
   //
   auto name = FindPos(args.back()->GetPos());
   if(name.pos == string::npos) return source_.End();
   return FindFirstOf(name, ")");
}

//------------------------------------------------------------------------------

SourceLoc Editor::FindFirstOf(const SourceLoc& loc, const string& chars)
{
   Debug::ft("Editor.FindFirstOf");

   source_.Reposition(loc);
   return source_.FindFirstOf(chars);
}

//------------------------------------------------------------------------------

SourceIter Editor::FindFuncDeclLoc
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

SourceIter Editor::FindFuncDefnLoc(const CodeFile* file,
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
         return Code().end();
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

SourceIter Editor::FindLine(size_t line)
{
   Debug::ft("Editor.FindLine");

   for(auto s = Code().begin(); s != Code().end(); ++s)
   {
      if(s->line == line) return s;
   }

   return Code().end();
}

//------------------------------------------------------------------------------

SourceIter Editor::FindLine(size_t line, string& expl)
{
   Debug::ft("Editor.FindLine(expl)");

   auto s = FindLine(line);

   if(s == Code().end())
   {
      expl = "Line " + std::to_string(line + 1) + " not found.";
      expl.push_back(CRLF);
   }

   return s;
}

//------------------------------------------------------------------------------

CodeWarning* Editor::FindLog
   (const CodeWarning& log, const CxxNamed* item, word offset)
{
   Debug::ft("Editor.FindLog");

   for(auto w = warnings_.begin(); w != warnings_.end(); ++w)
   {
      if((w->warning_ == log.warning_) && (w->item_ == item) &&
         (w->offset_ == offset))
      {
         return &*w;
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

SourceLoc Editor::FindNonBlank(SourceIter iter, size_t pos)
{
   Debug::ft("Editor.FindNonBlank");

   while(iter != Code().end())
   {
      pos = iter->code.find_first_not_of(WhitespaceChars, pos);
      if(pos != string::npos) return SourceLoc(iter, pos);
      ++iter;
      pos = 0;
   }

   return SourceLoc(iter);
}

//------------------------------------------------------------------------------

SourceLoc Editor::FindPos(size_t pos)
{
   Debug::ft("Editor.FindPos");

   //  Find ITEM's location within its file, and convert this to a line
   //  number and offset on that line.
   //
   auto& lexer = file_->GetLexer();
   auto line = lexer.GetLineNum(pos);
   if(line == string::npos) return source_.End();
   auto iter = FindLine(line);
   if(iter == Code().end()) return SourceLoc(iter);
   auto start = lexer.GetLineStart(line);
   return SourceLoc(iter, pos - start);
}

//------------------------------------------------------------------------------

SourceLoc Editor::FindSigEnd(const CodeWarning& log)
{
   Debug::ft("Editor.FindSigEnd(log)");

   if(log.item_ == nullptr) return source_.End();
   if(log.item_->Type() != Cxx::Function) return source_.End();
   return FindSigEnd(static_cast< const Function* >(log.item_));
}

//------------------------------------------------------------------------------

SourceLoc Editor::FindSigEnd(const Function* func)
{
   Debug::ft("Editor.FindSigEnd(func)");

   //  Look for the first semicolon or left brace after the function's name.
   //
   auto loc = FindPos(func->GetPos());
   if(loc.pos == string::npos) return source_.End();
   return FindFirstOf(loc, ";{");
}

//------------------------------------------------------------------------------

SourceIter Editor::FindSpecialFuncLoc
   (const CodeWarning& log, FuncDeclAttrs& attrs)
{
   Debug::ft("Editor.FindSpecialFuncLoc");

   //  Find special member functions defined in the class that needs
   //  to have another one added.  The insertion point will be either
   //  right after the class declaration or one of these functions.
   //  The order is constructor, destructor, copy constructor, and
   //  finally copy operator.
   //
   auto where = FindPos(log.item_->GetPos());
   if(where.pos == string::npos) return Code().end();
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
      return Code().end();
   };

   if(func == nullptr)
   {
      //  Insert the new function at the beginning of the class.
      //  Skip over, or insert, a "public" access control.
      //
      where = FindFirstOf(where, "{");
      ++where.iter;

      if(where.iter->code.find(PUBLIC_STR) == string::npos)
      {
         SourceLine control(PUBLIC_STR, SIZE_MAX);
         control.code.push_back(':');
         where.iter = Code().insert(where.iter, control);
         ++where.iter;
      }

      //  If a comment will follow the new function, add a comment
      //  for it as well.
      //
      auto type = GetLineType(where.iter);
      auto& line = LineTypeAttr::Attrs[type];
      attrs.comment = (!line.isCode && (type != BlankLine));
   }
   else
   {
      //  Insert the new function after FUNC.  If a comment precedes
      //  FUNC, also add one for the new function.
      //
      where.iter = LineAfterFunc(func);
      auto floc = FindPos(func->GetPos());
      auto type = GetLineType(std::prev(floc.iter));
      auto& line = LineTypeAttr::Attrs[type];
      attrs.comment = (!line.isCode && (type != BlankLine));
   }

   //  Decide where to insert a blank line to offset the new function.
   //  LineAfterFunc often returns a blank line, so advance to the next
   //  one.  We want to return a non-blank line so that its indentation
   //  can determine the indentation for the new code.
   //
   if(where.iter->code.find_first_not_of(WhitespaceChars) == string::npos)
   {
      ++where.iter;
   }

   //  Where to insert a blank line depends on whether the new function
   //  will appear first, last, or somewhere in the middle of its class.
   //
   auto prevType = GetLineType(std::prev(where.iter));
   auto nextType = GetLineType(where.iter);

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
         (GetLineType(std::next(where.iter)) != BlankLine))
      {
         attrs.blank = BlankNone;
      }
   }

   return where.iter;
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

SourceLoc Editor::FindWord
   (SourceIter iter, size_t pos, const string& id, size_t* range)
{
   Debug::ft("Editor.FindWord");

   //  Look for ID, which must be preceded and followed by characters
   //  that are not allowed in an identifier.
   //
   while(iter != Code().end())
   {
      if(pos < iter->code.size())
      {
         pos = iter->code.find(id, pos);

         while(pos != string::npos)
         {
            auto prev = true;
            auto next = true;
            if(pos > 0)
               prev = !IsWordChar(iter->code[pos - 1]);
            if(pos + id.size() < iter->code.size())
               next = !IsWordChar(iter->code[pos + id.size()]);
            if(prev && next) return SourceLoc(iter, pos);
            pos = iter->code.find(id, pos + 1);
         }
      }

      if((range == nullptr) || (--*range == 0))
         return source_.End();
      ++iter;
      pos = 0;
   }

   return source_.End();
}

//------------------------------------------------------------------------------

fixed_string FixPrompt = "  Fix?";

word Editor::Fix(CliThread& cli, const FixOptions& opts, string& expl)
{
   Debug::ft("Editor.Fix");

   //  Get the file's warnings and source code.
   //
   if(Code().empty())
   {
      *cli.obuf << "Failed to load source code for " << file_->Name() << CRLF;
      return -1;
   }

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
      DisplayLog(cli, *item, first);
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
         auto logs = item->LogsToFix(expl);

         if(!expl.empty())
         {
            *cli.obuf << spaces(2) << expl << CRLF;
            expl.clear();
         }

         for(auto log = logs.begin(); log != logs.end(); ++log)
         {
            auto editor = (*log)->file_->GetEditor(expl);
            if(editor != nullptr) rc = editor->FixLog(cli, **log, expl);
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

   //  Write out the modified file(s).
   //
   auto err = false;

   for(auto editor = Editors_.begin(); editor != Editors_.end(); ++editor)
   {
      if((*editor)->Write(expl) != 0) err = true;
      *cli.obuf << spaces(2) << expl;
   }

   Commits_ += Editors_.size();
   Editors_.clear();

   //  A result of -1 or greater indicates that the next file can still be
   //  processed, so return a lower value if the user wants to quit or if
   //  an error occurred when writing a file.  On a quit, a "committed"
   //  message has been displayed for the last file written, and it will
   //  be displayed again (because of rc < 0) if not cleared.
   //
   if(err)
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
      return EraseFunction(func, expl);
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
   case FunctionCouldBeMember:
      return ChangeFunctionToMember(func, log.offset_, expl);
   case CouldBeNoexcept:
      return TagAsNoexcept(func, expl);
   case ShouldNotBeNoexcept:
      return EraseNoexceptTag(func, expl);
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
      auto editor = file->GetEditor(expl);
      if(editor != nullptr) rc = editor->FixFunction(*f, log, expl);

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
   return Unimplemented(expl);  //*
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
      return EraseCode(log.pos_, expl);
   case IncludeAdd:
      return InsertInclude(log, expl);
   case IncludeRemove:
      return EraseCode(log.pos_, expl);
   case RemoveOverrideTag:
      return EraseOverrideTag(log, expl);
   case UsingInHeader:
      return ReplaceUsing(log, expl);
   case UsingDuplicated:
      return EraseCode(log.pos_, ";", expl);
   case UsingAdd:
      return InsertUsing(log, expl);
   case UsingRemove:
      return EraseCode(log.pos_, ";", expl);
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
      return EraseCode(log.pos_, ";", expl);
   case EnumeratorUnused:
      return EraseEnumerator(log, expl);
   case FriendUnused:
      return EraseCode(log.pos_, ";", expl);
   case FunctionUnused:
      return FixFunctions(cli, log, expl);
   case TypedefUnused:
      return EraseCode(log.pos_, ";", expl);
   case ForwardUnresolved:
      return EraseForward(log, expl);
   case FriendUnresolved:
      return EraseCode(log.pos_, ";", expl);
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
      return EraseCode(log.pos_, ";", expl);
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
      return TagAsDefaulted(log, expl);
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

   if(Code().empty())
   {
      expl = "Failed to load source code for " + file_->Name();
      expl.push_back(CRLF);
      return -1;
   }

   EraseTrailingBlanks();
   EraseBlankLinePairs();
   ConvertTabsToBlanks();
   return Write(expl);
}

//------------------------------------------------------------------------------

LineType Editor::GetLineType(const SourceIter& iter) const
{
   bool cont;
   std::set< Warning > warnings;

   if(iter->line != SIZE_MAX) return file_->GetLineType(iter->line);
   return file_->ClassifyLine(iter->code, cont, warnings);
}

//------------------------------------------------------------------------------

SourceIter Editor::IncludesBegin()
{
   Debug::ft("Editor.IncludesBegin");

   for(auto s = Code().begin(); s != Code().end(); ++s)
   {
      if(s->code.find(HASH_INCLUDE_STR) == 0) return s;
   }

   return Code().end();
}

//------------------------------------------------------------------------------

SourceIter Editor::IncludesEnd()
{
   Debug::ft("Editor.IncludesEnd");

   for(auto s = IncludesBegin(); s != Code().end(); ++s)
   {
      if(s->code.find(HASH_INCLUDE_STR) == 0) continue;
      if(s->code.find_first_not_of(WhitespaceChars) == string::npos) continue;

      //  We have found something else.  Back up to the last #include
      //  and return the line that follows it.
      //
      --s;
      while(s->code.find_first_not_of(WhitespaceChars) == string::npos) --s;
      return ++s;
   }

   return Code().end();
}

//------------------------------------------------------------------------------

size_t Editor::Indent(const SourceIter& iter, bool split)
{
   Debug::ft("Editor.Indent");

   //  Start by erasing all blanks at the beginning of this line.
   //
   auto pos = iter->code.find_first_not_of(WhitespaceChars);
   if(pos == string::npos) return string::npos;
   iter->code.erase(0, pos);
   if(pos > 0) Changed();

   //  Get the line's depth.  If it isn't part of the original code,
   //  consult previous lines until one known to the lexer is found.
   //
   size_t depth = SIZE_MAX;
   auto curr = iter;

   for(NO_OP; curr->line == SIZE_MAX; --curr)
   {
      if(curr == Code().begin())
      {
         depth = 0;
         break;
      }
   }

   if(depth == SIZE_MAX)
   {
      depth = file_->GetDepth(curr->line);
   }

   if(split)
   {
      //  Indent one level unless the new line starts with a left brace.
      //
      auto first = iter->code.find_first_not_of(WhitespaceChars);
      if((first != string::npos) && (iter->code[first] != '{')) ++depth;
   }

   auto indent = depth * file_->IndentSize();
   iter->code.insert(0, indent, SPACE);
   Changed();
   return indent;
}

//------------------------------------------------------------------------------

word Editor::InitByCtorCall(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InitByCtorCall");

   //  Change ["const"] <type> <name> = <class> "(" [<args>] ");"
   //      to ["const"] <class> <name> "(" <args> ");"
   //
   auto loc = FindPos(log.pos_);
   if(loc.pos == string::npos) return NotFound(expl, "Statement");
   auto name = FindWord(loc.iter, 0, *log.item_->Name());
   if(name.pos == string::npos) return NotFound(expl, "Variable name");
   auto vprev = name.iter->code.rfind(SPACE, name.pos);
   if(vprev == string::npos) return NotFound(expl, "Start of variable name");
   auto eq = FindFirstOf(name, "=");
   if(eq.pos == string::npos) return NotFound(expl, "Assignment operator");
   auto vend = RfindNonBlank(eq.iter, eq.pos - 1);
   if(vend.pos == string::npos) return NotFound(expl, "End of variable name");
   auto lpar = FindFirstOf(eq, "(");
   if(lpar.pos == string::npos) return NotFound(expl, "Left parenthesis");
   auto semi = FindFirstOf(eq, ";");
   if(semi.pos == string::npos) return NotFound(expl, "Semicolon");
   auto rpar = semi.iter->code.rfind(')');
   if(rpar == string::npos) return NotFound(expl, "Right parenthesis");
   auto cbegin = FindNonBlank(eq.iter, eq.pos + 1);
   if(cbegin.pos == string::npos) return NotFound(expl, "Start of class name");
   auto cend = RfindNonBlank(lpar.iter, lpar.pos - 1);
   if(cend.pos == string::npos) return NotFound(expl, "End of class name");
   if(cbegin.iter != cend.iter) return Report(expl, "Class name spans lines");

   //  We can now form CODE, which will precede the argument list.
   //
   auto var = name.iter->code.substr(vprev + 1, vend.pos - vprev);
   auto cls = cbegin.iter->code.substr(cbegin.pos, cend.pos - cbegin.pos + 1);
   auto code = cls + SPACE + var;
   if(log.item_->IsConst()) code.insert(0, "const ");

   //  If the constructor is called with an empty argument list, delete the
   //  parentheses.  That is, auto name = class(); must become class name;
   //
   if((lpar.iter == semi.iter) &&
      (lpar.iter->code.find_first_not_of(WhitespaceChars, lpar.pos + 1) == rpar))
   {
      lpar.iter->code.erase(lpar.pos, rpar - lpar.pos + 1);
   }

   //  Erase whatever precedes the arguments.
   //
   auto indent = lpar.iter->code.find_first_not_of(WhitespaceChars);
   lpar.iter->code.erase(indent, lpar.pos - indent);

   if(name.iter == lpar.iter)
   {
      name.iter->code.insert(indent, code);
   }
   else
   {
      //  The arguments are on a separate line.  The first line can therefore be
      //  replaced, in its entirety, by CODE.  The lines might also be combined.
      //
      indent = name.iter->code.find_first_not_of(WhitespaceChars);
      name.iter->code.erase(indent);
      name.iter->code.append(code);
      EraseLineBreak(name.iter);
   }

   return Changed(name.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::InlineDebugFtName(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InlineDebugFtName");

   //  Find the fn_name data, in-line its string literal in the Debug::ft
   //  call, and then erase it.
   //
   auto s = FindLine(log.line_, expl);
   if(s == Code().end()) return 0;

   auto lpar = s->code.find('(');
   if(lpar == string::npos) return NotFound(expl, "Left parenthesis");
   auto rpar = s->code.find(')', lpar);
   if(rpar == string::npos) return NotFound(expl, "Right parenthesis");
   auto data = static_cast< const Data* >(log.item_);
   if(data == nullptr) return NotFound(expl, "fn_name declaration");

   string fname;
   if(!data->GetStrValue(fname)) return NotFound(expl, "fn_name definition");
   auto dloc = FindPos(data->GetPos());
   if(dloc.iter == Code().end()) return NotFound(expl, "fn_name in source");
   auto split = (dloc.LastChar() != ';');
   auto next = Code().erase(dloc.iter);
   if(split) Code().erase(next);
   auto literal = QUOTE + fname + QUOTE;
   s->code.replace(lpar + 1, rpar - lpar - 1, literal);
   return Changed(s, expl);
}

//------------------------------------------------------------------------------

SourceIter Editor::Insert(const SourceIter& iter, string code)
{
   Debug::ft("Editor.Insert");

   Changed();
   if(code.empty() || (code.back() != CRLF)) code.push_back(CRLF);
   return Code().insert(iter, SourceLine(code, SIZE_MAX));
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

   auto s = FindLine(log.line_, expl);
   if(s == Code().end()) return 0;
   Insert(s, EMPTY_STR);
   return 0;
}

//------------------------------------------------------------------------------

word Editor::InsertCopyCtorCall(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertCopyCtorCall");

   //  Have this copy constructor invoke its base class copy constructor.
   //
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::InsertDataInit(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertDataInit");

   //  Initialize this data item to its default value.
   //
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::InsertDebugFtCall
   (CliThread& cli, const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertDebugFtCall");

   auto name = FindPos(log.item_->GetPos());
   if(name.pos == string::npos) return NotFound(expl, "Function name");
   auto left = FindFirstOf(name, "{");
   if(left.pos == string::npos) return NotFound(expl, "Left brace");

   //  Get the start of the name for an fn_name declaration and the inline
   //  string literal for the Debug::ft invocation.  Set EXTRA if anything
   //  follows the left brace on the same line.
   //
   string flit, fvar;
   DebugFtNames(static_cast< const Function* >(log.item_), flit, fvar);
   auto extra = (left.iter->code.find_first_not_of
      (WhitespaceChars, left.pos + 1) != string::npos);

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
   auto stop = FindPos(right);

   for(auto loc = left; (loc != stop) && arg.empty(); ++loc.iter)
   {
      auto start = loc.iter->code.find(fvar);

      if(start != string::npos)
      {
         auto end = loc.iter->code.find_first_not_of(ValidNextChars, start);
         if(end == string::npos) return NotFound(expl, "End of fn_name");
         arg = loc.iter->code.substr(start, end - start);
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

   //  Create the call to Debug::ft at the top of the function, after its left
   //  brace.  If the left brace wasn't on a new line, push it down first.  If
   //  something followed the left brace, push it down as well.  Insert a blank
   //  line after the call to Debug::ft.
   //
   if(left.iter->code.find_first_not_of(WhitespaceChars) != left.pos)
      left = InsertLineBreak(left.iter, left.pos);
   if(extra) InsertLineBreak(left.iter, left.pos + 1);
   auto below = std::next(left.iter);
   below = Insert(below, EMPTY_STR);
   auto call = DebugFtCode(arg);
   below = Insert(below, call);
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
   if(curr == Code().end()) return NotFound(expl, "Missing function's class");

   //  Indent the code to match that at the insertion point unless the
   //  insertion point is a right brace.
   //
   string prefix;
   auto indent = curr->code.find_first_not_of(WhitespaceChars);
   if(indent == string::npos)
      prefix = spaces(file_->IndentSize());
   else if(curr->code.at(indent) == '}')
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
      curr = Insert(curr, EMPTY_STR);
   }

   auto func = Insert(curr, code);

   if(attrs.comment)
   {
      code = prefix + COMMENT_STR;
      curr = Insert(func, code);

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

      curr = Insert(curr, code);
   }

   if(attrs.blank == BlankBefore)
   {
      curr = Insert(curr, EMPTY_STR);
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
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::InsertEnumName(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertEnumName");

   //  Prompt for the enum's name and insert it.
   //
   return Unimplemented(expl);  //*
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

   for(auto s = PrologEnd(); s != Code().end(); ++s)
   {
      if(s->code.find(NAMESPACE_STR) == 0)
      {
         //  If this namespace matches NSPACE, add the declaration to it.
         //  If this namespace's name is alphabetically after NSPACE, add
         //  the declaration before it, along with its namespace.
         //
         auto comp = strCompare(s->code, nspace);
         if(comp == 0) return InsertForward(s, forward, expl);
         if(comp > 0) return InsertNamespaceForward(s, nspace, forward, expl);
      }
      else if((s->code.find(USING_STR) == 0) || (s == begin))
      {
         //  We have now passed any existing forward declarations, so add
         //  the new declaration here, along with its namespace.
         //
         return InsertNamespaceForward(s, nspace, forward, expl);
      }
   }

   return Report(expl, "Failed to insert forward declaration.");
}

//------------------------------------------------------------------------------

word Editor::InsertForward
   (const SourceIter& iter, const string& forward, string& expl)
{
   Debug::ft("Editor.InsertForward(iter)");

   //  ITER references a namespace that matches the one for a new forward
   //  declaration.  Insert the new declaration alphabetically within the
   //  declarations that already appear in this namespace.  Note that it
   //  may already have been inserted while fixing another warning.
   //
   for(auto s = std::next(iter, 2); s != Code().end(); ++s)
   {
      auto comp = strCompare(s->code, forward);
      if(comp == 0) return Report(expl, "Previously inserted.");

      if((comp > 0) || (s->code.find('}') != string::npos))
      {
         s = Insert(s, forward);
         return Changed(s, expl);
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

   //  Modify the characters that surround the #include's file name
   //  based on the group to which it belongs.
   //
   if(MangleInclude(include, expl) != 0) return 0;

   //  Insert the new #include in its sort order.
   //
   auto end = IncludesEnd();

   for(auto s = IncludesBegin(); s != end; ++s)
   {
      if(s->code.find_first_not_of(WhitespaceChars) == string::npos)
      {
         continue;
      }

      if(!IncludesAreSorted(s->code, include))
      {
         s = Insert(s, include);
         return Changed(s, expl);
      }
   }

   //  Add the new #include to the end of the list.  If there is no
   //  blank line at the end of the list, add one.
   //
   auto s = end;

   if(end->code.find_first_not_of(WhitespaceChars) != string::npos)
   {
      s = Insert(end, EMPTY_STR);
   }

   s = Insert(s, include);
   return Changed(s, expl);
}

//------------------------------------------------------------------------------

word Editor::InsertIncludeGuard(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertIncludeGuard");

   //  Insert the guard before the first #include.  If there isn't one,
   //  insert it before the first line of code.  If there isn't any code,
   //  insert it at the end of the file.
   //
   auto where = IncludesBegin();
   if(where == Code().end()) where = PrologEnd();
   string guardname = log.file_->MakeGuardName();
   string code = "#define " + guardname;
   where = Insert(where, EMPTY_STR);
   where = Insert(where, code);
   code = "#ifndef " + guardname;
   where = Insert(where, code);
   Insert(Code().end(), HASH_ENDIF_STR);
   return Changed(where, expl);
}

//------------------------------------------------------------------------------

SourceLoc Editor::InsertLineBreak(const SourceIter& iter, size_t pos)
{
   Debug::ft("Editor.InsertLineBreak(pos)");

   if(iter->code.size() <= pos) return source_.End();
   if(iter->code.find_first_not_of(WhitespaceChars, pos) == string::npos)
      return SourceLoc(iter, pos);
   auto code = iter->code.substr(pos);
   iter->code.erase(pos);
   auto next = Insert(std::next(iter), code);
   auto indent = Indent(next, true);
   return SourceLoc(next, indent);
}

//------------------------------------------------------------------------------

word Editor::InsertLineBreak(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertLineBreak(log)");

   //  Consider parentheses, lexical level, binary operators...
   //
   auto s = FindLine(log.line_, expl);
   if(s == Code().end()) return 0;

   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::InsertMemberInit(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertMemberInit");

   //  Initialize the member to its default value.
   //
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::InsertNamespaceForward(const SourceIter& iter,
   const string& nspace, const string& forward, string& expl)
{
   Debug::ft("Editor.InsertNamespaceForward");

   //  Insert a new forward declaration, along with an enclosing namespace,
   //  at ITER.  Offset it with blank lines.
   //
   auto s = Insert(iter, EMPTY_STR);
   s = Insert(s, "}");
   auto f = Insert(s, forward);
   s = Insert(f, "{");
   s = Insert(s, nspace);
   s = Insert(s, EMPTY_STR);
   return Changed(f, expl);
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
   auto s1 = FindFuncDeclLoc(cls, name, decl);
   if(s1 == Code().end()) return -1;

   auto file = FindFuncDefnFile(cli, cls, name);
   if(file == nullptr) return -1;

   auto editor = file->GetEditor(expl);
   if(editor == nullptr) return -1;

   FuncDefnAttrs defn;
   auto s2 = editor->FindFuncDefnLoc(file, cls, name, expl, defn);
   if(s2 == editor->Source().end()) return -1;

   //  Insert the function's declaration and definition.
   //
   InsertPatchDecl(s1, decl);
   editor->InsertPatchDefn(s2, cls, defn);
   return 0;
}

//------------------------------------------------------------------------------

fixed_string PatchComment = "Overridden for patching.";
fixed_string PatchReturn = "void";
fixed_string PatchSignature = "Patch(sel_t selector, void* arguments)";
fixed_string PatchInvocation = "Patch(selector, arguments)";

void Editor::InsertPatchDecl(SourceIter& iter, const FuncDeclAttrs& attrs)
{
   Debug::ft("Editor.InsertPatchDecl");

   //  See if a blank line should be inserted below the function.  Then
   //  insert its definition and comment, and see if a blank line and/or
   //  access control keyword should be inserted above the function.
   //
   if(attrs.blank == BlankAfter) iter = Insert(iter, EMPTY_STR);

   string code = PatchReturn;
   code.push_back(SPACE);
   code.append(PatchSignature);
   code.push_back(SPACE);
   code.append(OVERRIDE_STR);
   code.push_back(';');
   iter = Insert(iter, strCode(code, 1));

   if(attrs.comment)
   {
      iter = Insert(iter, strComment(EMPTY_STR, 1));
      iter = Insert(iter, strComment(PatchComment, 1));
   }

   if(attrs.access != Cxx::Access_N)
   {
      std::ostringstream access;
      access << attrs.access << ':';
      iter = Insert(iter, access.str());
   }
   else if(attrs.blank == BlankBefore)
   {
      iter = Insert(iter, EMPTY_STR);
   }
}

//------------------------------------------------------------------------------

void Editor::InsertPatchDefn
   (SourceIter& iter, const Class* cls, const FuncDefnAttrs& attrs)
{
   Debug::ft("Editor.InsertPatchDefn");

   //  See whether to insert a blank and rule below the function.
   //
   if(attrs.blank == BlankAfter)
   {
      iter = Insert(iter, EMPTY_STR);

      if(attrs.rule)
      {
         iter = Insert(iter, strRule('-'));
         iter = Insert(iter, EMPTY_STR);
      }
   }

   //  Add the closing brace, the call to the base class Patch function,
   //  the opening brace, and the function signature.
   //
   iter = Insert(iter, "}");
   auto base = cls->BaseClass();
   auto code = *base->Name();
   code.append(SCOPE_STR);
   code.append(PatchInvocation);
   code.push_back(';');
   iter = Insert(iter, strCode(code, 1));
   iter = Insert(iter, "{");
   code = PatchReturn;
   code.push_back(SPACE);
   code.append(*cls->Name());
   code.append(SCOPE_STR);
   code.append(PatchSignature);
   iter = Insert(iter, code);

   //  See whether to insert a blank and rule above the function.
   //
   if(attrs.blank == BlankBefore)
   {
      iter = Insert(iter, EMPTY_STR);

      if(attrs.rule)
         {
         iter = Insert(iter, strRule('-'));
         iter = Insert(iter, EMPTY_STR);
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
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

void Editor::InsertPrefix
   (const SourceIter& iter, size_t pos, const string& prefix)
{
   Debug::ft("Editor.InsertPrefix");

   auto first = iter->code.find_first_not_of(WhitespaceChars);

   if(pos + prefix.size() < first)
   {
      iter->code.replace(pos, prefix.size(), prefix);
   }
   else
   {
      iter->code.insert(pos, 1, SPACE);
      iter->code.insert(pos, prefix);
   }

   Changed();
}

//------------------------------------------------------------------------------

word Editor::InsertPureVirtual(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.InsertPureVirtual");

   //  Insert a definition that invokes Debug::SwLog with strOver(this).
   //
   return Unimplemented(expl);  //*
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

   auto code = CodeBegin();
   auto usings = false;

   for(auto s = PrologEnd(); s != Code().end(); ++s)
   {
      if(s->code.find(USING_STR) == 0)
      {
         //  If this using statement is alphabetically after STATEMENT,
         //  add the new statement before it.
         //
         usings = true;

         if(strCompare(s->code, statement) > 0)
         {
            s = Insert(s, statement);
            return Changed(s, expl);
         }
      }
      else if((usings && CodeIsEmpty(s)) || (s == code))
      {
         //  We have now passed any existing using statements, so add
         //  the new statement here.  If it is the first one, precede
         //  it with a blank line.
         //
         s = Insert(s, EMPTY_STR);
         auto u = Insert(s, statement);
         if(!usings) s = Insert(u, EMPTY_STR);
         return Changed(u, expl);
      }
   }

   return Report(expl, "Failed to insert using statement.");
}

//------------------------------------------------------------------------------

SourceIter Editor::IntroStart(const SourceIter& iter, bool funcName)
{
   Debug::ft("Editor.IntroStart");

   SourceIter curr = iter;

   while(curr != Code().begin())
   {
      auto type = GetLineType(--curr);

      if(type == SeparatorComment)
      {
         //  Don't include a separator or a blank that follows it.
         //
         ++curr;
         if(LineTypeAttr::Attrs[GetLineType(curr)].isBlank) ++curr;
         return curr;
      }

      if(LineTypeAttr::Attrs[type].isCode)
      {
         //  The only code that can be included is an fn_name.
         //
         if(funcName && (type == FunctionName)) continue;
         return ++curr;
      }
   }

   return Code().begin();
}

//------------------------------------------------------------------------------

SourceIter Editor::LineAfterFunc(const Function* func)
{
   Debug::ft("Editor.LineAfterFunc");

   SourceLoc end(Code().end());
   size_t begin, close;

   auto open = func->GetRange(begin, close);

   if(open != string::npos)
   {
      //  This is a function definition, and CLOSE is the position
      //  of its final right brace.
      //
      end = FindPos(close);
   }
   else
   {
      //  This is a function declaration.  Find the location of its
      //  name, and then the semicolon or right brace at its end.
      //
      end = FindPos(func->GetPos());
      if(end.pos == string::npos) return Code().end();
      end = FindFirstOf(end, ";}");
   }

   if(end.pos == string::npos) return Code().end();
   return std::next(end.iter);
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

word Editor::MoveDefine(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.MoveDefine");

   //  Move the #define directly after the #include directives.
   //
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::MoveFunction(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.MoveFunction");

   //  Move the function's definition to the correct location.
   //
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::MoveMemberInit(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.MoveMemberInit");

   //  Move the member to the correct location in the initialization list.
   //
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

SourceIter Editor::PrologEnd()
{
   Debug::ft("Editor.PrologEnd");

   for(auto s = Code().begin(); s != Code().end(); ++s)
   {
      if(LineTypeAttr::Attrs[GetLineType(s)].isCode) return s;
   }

   return Code().end();
}

//------------------------------------------------------------------------------

void Editor::QualifyReferent(const CxxNamed* item, const CxxNamed* ref)
{
   Debug::ft("Editor.QualifyReferent");

   //  Within ITEM, prefix NS wherever SYMBOL appears as an identifier.
   //
   string expl;
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

   size_t pos, end;
   item->GetRange(pos, end);
   auto lexer = file_->GetLexer();
   lexer.Reposition(pos);
   string name;

   while((lexer.Curr() < end) && lexer.FindIdentifier(name, false))
   {
      if(name != *symbol)
      {
         lexer.Advance(name.size());
         continue;
      }

      //  This line contains at least one occurrence of NAME.  Qualify each
      //  occurrence with NS if it is not already qualified.
      //
      auto line = lexer.GetLineNum(lexer.Curr());
      auto s = FindLine(line, expl);
      if(s == Code().end()) continue;

      for(auto loc = FindWord(s, 0, name); loc.pos != string::npos;
          loc = FindWord(loc.iter, loc.pos, name))
      {
         if(s->code.rfind(SCOPE_STR, loc.pos) != loc.pos - 2)
         {
            auto qual = ns->ScopedName(false);
            qual.append(SCOPE_STR);
            s->code.insert(loc.pos, qual);
            Changed();
            loc.pos += qual.size();
         }

         loc.pos += name.size();  // always bypass current occurrence of NAME
      }

      //  Advance to the next line.
      //
      pos = lexer.FindLineEnd(lexer.Curr());
      lexer.Reposition(pos);
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
   auto def = FindLine(log.line_, expl);
   if(def == Code().end()) return 0;
   auto ifn = Rfind(def, HASH_IFNDEF_STR);
   if(ifn.pos == string::npos) return NotFound(expl, HASH_IFNDEF_STR);
   if(def->code.find(HASH_DEFINE_STR) == string::npos)
      return NotFound(expl, HASH_DEFINE_STR);
   auto guard = log.file_->MakeGuardName();
   ifn.iter->code.erase(strlen(HASH_IFNDEF_STR) + 1);
   ifn.iter->code.append(guard);
   def->code.erase(strlen(HASH_DEFINE_STR) + 1);
   def->code.append(guard);
   return Changed(def, expl);
}

//------------------------------------------------------------------------------

word Editor::ReplaceHeading(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ReplaceHeading");

   //  Remove the existing header and replace it with the standard one,
   //  inserting the file name where appropriate.

   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::ReplaceName(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ReplaceName");

   //  Prompt for a new name that will replace the existing one.
   //
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::ReplaceNull(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ReplaceNull");

   //  If there are multiple occurrences on the same line, each one will
   //  cause a log, so just fix the first one.
   //
   auto s = FindLine(log.line_, expl);
   if(s == Code().end()) return 0;
   auto null = FindWord(s, 0, NULL_STR);
   if(null.pos == string::npos) return NotFound(expl, NULL_STR, true);
   null.iter->code.erase(null.pos, strlen(NULL_STR));
   null.iter->code.insert(null.pos, NULLPTR_STR);
   return Changed(null.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::ReplaceSlashAsterisk(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.ReplaceSlashAsterisk");

   auto s = FindLine(log.line_, expl);
   if(s == Code().end()) return 0;
   auto pos1 = s->code.find(COMMENT_BEGIN_STR);
   if(pos1 == string::npos) return NotFound(expl, COMMENT_BEGIN_STR);
   auto pos2 = s->code.find(COMMENT_END_STR);
   auto pos3 = s->code.find_first_not_of(WhitespaceChars, pos1 + 2);
   auto pos4 = string::npos;
   if(pos2 != string::npos)
      pos4 = s->code.find_first_not_of(WhitespaceChars, pos2 + 2);

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
      s->code.erase(pos1);
      Changed();
   }
   else if((pos2 == string::npos) && (pos3 != string::npos))  // [2]
   {
      s->code.replace(pos1, strlen(COMMENT_STR), COMMENT_STR);
      Changed();
   }
   else  // [3]
   {
      s->code.erase(pos2);
      s->code.replace(pos1, strlen(COMMENT_STR), COMMENT_STR);
      return Changed(s, expl);
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
   for(++s; s != Code().end(); ++s)
   {
      pos2 = s->code.find(COMMENT_END_STR);
      pos3 = s->code.find_first_not_of(WhitespaceChars);
      if(pos3 == pos2) pos3 = string::npos;

      if(pos2 != string::npos)
         pos4 = s->code.find_first_not_of(WhitespaceChars, pos2 + 2);
      else
         pos4 = string::npos;

      if(pos2 == string::npos)  // [1]
      {
         if(pos3 > pos1) pos3 = pos1;
         InsertPrefix(s, pos3, COMMENT_STR);
         Changed();
      }
      else if((pos3 == string::npos) && (pos4 == string::npos))  // [2]
      {
         s->code.erase(pos2);
         return Changed(s, expl);
      }
      else if((pos3 == string::npos) && (pos4 != string::npos))  // [3]
      {
         s->code.erase(pos2, strlen(COMMENT_END_STR));
         Changed();
         auto loc = InsertLineBreak(s, pos2);
         return Changed(loc.iter, expl);
      }
      else if((pos3 != string::npos) && (pos4 == string::npos))  // [4]
      {
         s->code.erase(pos2);
         if(pos3 > pos1) pos3 = pos1;
         InsertPrefix(s, pos3, COMMENT_STR);
         return Changed(s, expl);
      }
      else  // [5]
      {
         s->code.erase(pos2, strlen(COMMENT_END_STR));
         InsertLineBreak(s, pos2);
         if(pos3 > pos1) pos3 = pos1;
         InsertPrefix(s, pos3, COMMENT_STR);
         return Changed(s, expl);
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
   return EraseCode(log.pos_, ";", expl);
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

SourceLoc Editor::Rfind(SourceIter iter, const string& str, size_t off)
{
   Debug::ft("Editor.Rfind");

   if(iter == Code().end()) return SourceLoc(iter);

   auto pos = iter->code.rfind(str, off);

   while(pos == string::npos)
   {
      if(iter == Code().begin()) return source_.End();
      --iter;
      pos = iter->code.rfind(str);
   }

   return SourceLoc(iter, pos);
}

//------------------------------------------------------------------------------

SourceLoc Editor::RfindNonBlank(SourceIter iter, size_t pos)
{
   Debug::ft("Editor.RfindNonBlank");

   if(iter == Code().end()) return SourceLoc(iter);
   if(pos == string::npos) pos = iter->code.size() - 1;

   while(true)
   {
      pos = RfindFirstNotOf(iter->code, pos, WhitespaceChars);
      if(pos != string::npos) return SourceLoc(iter, pos);
      if(iter == Code().begin()) break;
      --iter;
      pos = iter->code.size();
   }

   return source_.End();
}

//------------------------------------------------------------------------------

word Editor::SortIncludes(string& expl)
{
   Debug::ft("Editor.SortIncludes");

   //  std::list does not support a sort bounded by iterators, so move all of
   //  the #include statements into a new list, sort them, and reinsert them.
   //  BEGIN is the line above the first #include.
   //
   auto begin = Code().cend();
   std::list< SourceLine > includes;

   for(auto s = Code().cbegin(); s != Code().cend(); NO_OP)
   {
      if(s->code.find(HASH_INCLUDE_STR) == 0)
      {
         if((begin == Code().cend()) &&
            (s != Code().cbegin()))
         {
            begin = std::prev(s);
         }

         includes.push_back(SourceLine(s->code, s->line));
         s = Code().erase(s);
      }
      else
      {
         ++s;
      }
   }

   if(includes.empty()) return NotFound(expl, HASH_INCLUDE_STR);

   //  Reinsert the #includes after the line where they originally appeared.
   //  If BEGIN is still cend(), the first line in the file was an #include.
   //
   if(begin != Code().cend())
      ++begin;
   else
      begin = Code().cbegin();

   includes.sort(IncludesSorted);

   for(auto s = includes.begin(); s != includes.end(); ++s)
   {
      Code().insert(begin, *s);
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
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

word Editor::TagAsConstArgument(const Function* func, word offset, string& expl)
{
   Debug::ft("Editor.TagAsConstArgument");

   //  Find the line on which the argument's type appears, and insert
   //  "const" before that type.
   //
   auto& args = func->GetArgs();
   auto index = func->LogOffsetToArgIndex(offset);
   auto arg = args.at(index).get();
   if(arg == nullptr) return NotFound(expl, "Argument");
   auto type = FindPos(arg->GetTypeSpec()->GetPos());
   if(type.pos == string::npos) return NotFound(expl, "Argument type");
   type.iter->code.insert(type.pos, "const ");
   return Changed(type.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsConstData(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.TagAsConstData");

   //  Find the line on which the data's type appears, and insert
   //  "const" before that type.
   //
   auto type = FindPos(log.item_->GetTypeSpec()->GetPos());
   if(type.pos == string::npos) return NotFound(expl, "Data type");
   type.iter->code.insert(type.pos, "const ");
   return Changed(type.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsConstFunction(const Function* func, string& expl)
{
   Debug::ft("Editor.TagAsConstFunction");

   //  Insert " const" after the right parenthesis at the end of the function's
   //  argument list.
   //
   auto endsig = FindArgsEnd(func);
   if(endsig.pos == string::npos) return NotFound(expl, "End of argument list");
   endsig.iter->code.insert(endsig.pos + 1, " const");
   return Changed(endsig.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsConstPointer(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.TagAsConstPointer");

   //  If there is more than one pointer, this applies to the last one, so
   //  back up from the data item's name.  Search for the name on this line
   //  in case other edits have altered its position.
   //
   auto data = FindPos(log.item_->GetPos());
   if(data.pos == string::npos) return NotFound(expl, "Data member");
   auto pos = data.iter->code.find(*log.item_->Name());
   if(pos == string::npos) return NotFound(expl, "Member name");
   auto ptr = Rfind(data.iter, "*", pos);
   if(ptr.pos == string::npos) return NotFound(expl, "Pointer tag");
   ptr.iter->code.insert(ptr.pos + 1, " const");
   return Changed(ptr.iter, expl);
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
   auto loc = FindPos(arg->GetPos());
   if(loc.pos == string::npos) return NotFound(expl, "Argument name");
   auto prev = RfindNonBlank(loc.iter, loc.pos - 1);
   prev.iter->code.insert(prev.pos + 1, 1, '&');
   auto rc = TagAsConstArgument(func, offset, expl);
   if(rc != 0) return rc;
   expl.clear();
   return Changed(prev.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsDefaulted(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.TagAsDefaulted");

   //  If this is a separate definition, delete it and tag the definition
   //  as defaulted.
   //
   auto defn = static_cast< const Function* >(log.item_);
   if(defn->GetDecl() != defn)
   {
      return EraseCode(log.pos_, "}", expl);
   }

   //  This is the function's declaration, and possibly its definition.
   //
   auto endsig = FindSigEnd(log);
   if(endsig.pos == string::npos) return NotFound(expl, "Signature end");
   if(endsig.iter->code.at(endsig.pos) == ';')
   {
      endsig.iter->code.insert(endsig.pos, " = default");
   }
   else
   {
      auto right = FindFirstOf(endsig, "}");
      if(right.pos == string::npos) return NotFound(expl, "Right brace");
      endsig.iter->code.erase(endsig.pos);
      if(right.iter != endsig.iter) right.iter->code.erase(0, right.pos + 1);
      endsig.iter->code.insert(endsig.pos, "= default;");
   }
   return Changed(endsig.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsExplicit(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.TagAsExplicit");

   //  A constructor can be tagged "constexpr", which the parser looks for
   //  only *after* "explicit".
   //
   auto ctor = FindPos(log.item_->GetPos());
   if(ctor.pos == string::npos) return NotFound(expl, "Constructor");
   auto prev = ctor.iter->code.rfind(CONSTEXPR_STR, ctor.pos);
   if(prev != string::npos) ctor.pos = prev;
   ctor.iter->code.insert(ctor.pos, "explicit ");
   return Changed(ctor.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsNoexcept(const Function* func, string& expl)
{
   Debug::ft("Editor.TagAsNoexcept");

   //  Insert "noexcept" after "const" but before "override" or "final".
   //  Start by finding the end of the function's argument list.
   //
   auto loc = FindPos(func->GetPos());
   if(loc.pos == string::npos) return NotFound(expl, "Function name");
   auto rpar = FindArgsEnd(func);
   if(rpar.pos == string::npos) return NotFound(expl, "End of argument list");

   //  If there is a "const" after the right parenthesis, insert "noexcept"
   //  after it, else insert "noexcept" after the right parenthesis.
   //
   auto cons = FindNonBlank(rpar.iter, rpar.pos + 1);
   if(cons.iter->code.find(CONST_STR, cons.pos) == cons.pos)
   {
      cons.iter->code.insert(cons.pos + strlen(CONST_STR), " noexcept");
      return Changed(cons.iter, expl);
   }
   rpar.iter->code.insert(rpar.pos + 1, " noexcept");
   return Changed(rpar.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsOverride(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.TagAsOverride");

   //  Insert "override" just before the end of the function's signature.
   //
   auto endsig = FindSigEnd(log);
   if(endsig.pos == string::npos) return NotFound(expl, "Signature end");

   //  Insert "override" after the previous non-blank character.
   //
   endsig = RfindNonBlank(endsig.iter, endsig.pos - 1);
   endsig.iter->code.insert(endsig.pos + 1, " override");
   return Changed(endsig.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsStaticFunction(const Function* func, string& expl)
{
   Debug::ft("Editor.TagAsStaticFunction");

   //  Start with the function's return type in case it's on the line above
   //  the function's name.  Then find the end of the argument list.
   //
   auto type = FindPos(func->GetTypeSpec()->GetPos());
   if(type.pos == string::npos) return NotFound(expl, "Function type");
   auto rpar = FindArgsEnd(func);
   if(rpar.pos == string::npos) return NotFound(expl, "End of argument list");

   //  Only a function's declaration, not its definition, is tagged "static".
   //  The parser also wants "static" to follow "extern" and "inline".
   //
   if(func->GetDecl() == func)
   {
      auto front = type.iter->code.rfind(INLINE_STR, type.pos);
      if(front != string::npos) type.pos = front;
      front = type.iter->code.rfind(EXTERN_STR, type.pos);
      if(front != string::npos) type.pos = front;
      type.iter->code.insert(type.pos, "static ");
      front = type.iter->code.rfind(VIRTUAL_STR, type.pos);
      if(front != string::npos)
         type.iter->code.erase(front, strlen(VIRTUAL_STR) + 1);
      Changed();
   }

   //  A static function cannot be const, so remove that tag if it exists.  If
   //  "const" is on the same line as RPAR, delete any space *before* "const";
   //  if it's on the next line, delete any space *after* "const" to preserve
   //  indentation.
   //
   if(func->IsConst())
   {
      auto tag = FindWord(rpar.iter, rpar.pos, CONST_STR);
      if(tag.pos != string::npos)
      {
         auto& code = tag.iter->code;
         code.erase(tag.pos, strlen(CONST_STR));

         if(rpar.iter == tag.iter)
         {
            if(IsBlank(code[tag.pos - 1])) code.erase(tag.pos - 1, 1);
         }
         else
         {
            if((tag.pos < code.size()) && IsBlank(code[tag.pos]))
               code.erase(tag.pos, 1);
         }
      }
   }

   return Changed(type.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::TagAsVirtual(const CodeWarning& log, string& expl)
{
   Debug::ft("Editor.TagAsVirtual");

   //  Make this destructor virtual.
   //
   return Unimplemented(expl);  //*
}

//------------------------------------------------------------------------------

void Editor::UpdateFuncDeclAttrs(const Function* func, FuncDeclAttrs& attrs)
{
   Debug::ft("Editor.UpdateFuncDeclAttrs");

   if(func == nullptr) return;

   //  If FUNC is commented, a blank line should also precede or follow it.
   //
   size_t begin, end;
   func->GetRange(begin, end);
   auto loc = FindPos(begin);
   auto type = GetLineType(std::prev(loc.iter));
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

   loc = FindPos(end);
   type = GetLineType(std::next(loc.iter));

   if(type == BlankLine)
   {
      attrs.blank = BlankBefore;
   }
}

//------------------------------------------------------------------------------

fn_name Editor_UpdateFuncDeclLoc = "Editor.UpdateFuncDeclLoc";

SourceIter Editor::UpdateFuncDeclLoc
   (const Function* prev, const Function* next, FuncDeclAttrs& attrs)
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
      return Code().end();
   }

   //  Insert the function before NEXT.  If the new function is to be offset
   //  with a blank, it will follow the new function.
   //
   if(attrs.blank != BlankNone) attrs.blank = BlankAfter;
   auto loc = FindPos(next->GetPos());
   auto pred = std::prev(loc.iter);

   while(true)
   {
      auto type = GetLineType(pred);

      if(!LineTypeAttr::Attrs[type].isCode)
      {
         if(type == BlankLine) break;

         //  This is a comment, so keep moving up to find the insertion point.
         //
         --pred;
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
   return ++pred;
}

//------------------------------------------------------------------------------

void Editor::UpdateFuncDefnAttrs(const Function* func, FuncDefnAttrs& attrs)
{
   Debug::ft("Editor.UpdateFuncDefnAttrs");

   if(func == nullptr) return;

   //  See if FUNC is preceded or followed by a rule and/or blank line.
   //
   bool blank = false;
   size_t begin, end;
   func->GetRange(begin, end);
   auto loc = FindPos(end);
   auto type = GetLineType(++loc.iter);

   if(type == BlankLine)
   {
      blank = true;
      type = GetLineType(++loc.iter);
      if(type == SeparatorComment)
      {
         attrs.rule = true;
         attrs.blank = BlankBefore;
         return;
      }
   }

   loc = FindPos(begin);

   while(true)
   {
      type = GetLineType(--loc.iter);

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

SourceIter Editor::UpdateFuncDefnLoc
   (const Function* prev, const Function* next, FuncDefnAttrs& attrs)
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

      //  Insert the function at the bottom of the file.  If the last line
      //  contains a closing brace, insert the definition above that.
      //
      auto last = std::prev(Code().end());
      auto pred = std::prev(last);
      auto type = GetLineType(pred);
      if(type == CloseBrace) return pred;
      return last;
   }

   //  Insert the function before NEXT.  If NEXT has an fn_name, insert the
   //  definition above *that*.  If the new function is to be offset with a
   //  blank and/or rule, they need to go *after* the new function.
   //
   if(attrs.blank != BlankNone) attrs.blank = BlankAfter;

   auto loc = FindPos(next->GetPos());
   auto pred = std::prev(loc.iter);

   while(true)
   {
      auto type = GetLineType(pred);

      switch(type)
      {
      case BlankLine:
         --pred;
         continue;
      case FunctionName:
         while(GetLineType(--pred) == FunctionName);
         return ++pred;
      default:
         return loc.iter;
      }
   }

   return loc.iter;
}

//------------------------------------------------------------------------------

word Editor::Write(string& expl)
{
   Debug::ft("Editor.Write");

   std::ostringstream stream;

   //  Perform an automatic >format on the file.  In particular, some edits
   //  could have introduced blank line pairs.
   //
   EraseTrailingBlanks();
   EraseBlankLinePairs();
   EraseEmptySeparators();
   EraseOffsets();
   ConvertTabsToBlanks();

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
   for(auto s = Code().begin(); s != Code().end(); ++s)
   {
      if(s->code.find(HASH_INCLUDE_STR) == 0)
      {
         DemangleInclude(s);
      }
   }

   for(auto s = Code().cbegin(); s != Code().cend(); ++s)
   {
      *output << s->code;

      if(s->code.empty() || (s->code.back() != CRLF))
      {
         *output << CRLF;  //@ if this occurs, find and fix root cause!
      }
   }

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

   for(auto item = warnings_.begin(); item != warnings_.end(); ++item)
   {
      if(item->status == Pending) item->status = Fixed;
   }

   stream << "..." << file_->Name() << " committed";
   return Report(expl, stream);
}
}
