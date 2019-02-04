//==============================================================================
//
//  Editor.cpp
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
#include "Editor.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <istream>
#include <iterator>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>
#include "CliThread.h"
#include "CodeFile.h"
#include "Cxx.h"
#include "CxxArea.h"
#include "CxxNamed.h"
#include "CxxScope.h"
#include "CxxScoped.h"
#include "CxxToken.h"
#include "Debug.h"
#include "Formatters.h"
#include "Lexer.h"
#include "NbCliParms.h"
#include "SysFile.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Characters that enclose the file name of an #include directive,
//  depending on the group to which it belongs.
//
const string FrontChars = "$%@!<\"";
const string BackChars = "$%@!>\"";

//  Returns true if C is a whitespace character.
//
bool IsBlank(char c);

//  Forward declarations of local functions.
//
//  Returns true if C is a character that may appear in an identifier.
//
bool IsWordChar(char c);

//  Sets EXPL to "TEXT not found."  If QUOTES is set, TEXT is enclosed in
//  quotes.  Returns 0.
//
word NotFound(string& expl, fixed_string text, bool quotes = false);

//  Sets EXPL to TEXT and returns RC.
//
word Report(string& expl, fixed_string text, word rc = 0);

//------------------------------------------------------------------------------

bool IsBlank(char c)
{
   return (WhitespaceChars.find_first_of(c) != string::npos);
}

//------------------------------------------------------------------------------

bool IsWordChar(char c)
{
   return (ValidNextChars.find_first_of(c) != string::npos);
}

//------------------------------------------------------------------------------

fn_name CodeTools_NotFound = "CodeTools.NotFound";

word NotFound(string& expl, fixed_string text, bool quotes)
{
   Debug::ft(CodeTools_NotFound);

   if(quotes) expl = QUOTE;
   expl += text;
   if(quotes) expl.push_back(QUOTE);
   expl += " not found.";
   return 0;
}

//------------------------------------------------------------------------------

fn_name CodeTools_Report = "CodeTools.Report";

word Report(string& expl, fixed_string text, word rc)
{
   Debug::ft(CodeTools_Report);

   expl = text;
   return rc;
}

//==============================================================================

fn_name Editor_ctor = "Editor.ctor";

Editor::Editor(const CodeFile* file, istreamPtr& input) :
   file_(file),
   input_(std::move(input)),
   line_(0),
   read_(1),
   changed_(false),
   sorted_(false),
   aliased_(false)
{
   Debug::ft(Editor_ctor);

   input_->clear();
   input_->seekg(0);
}

//------------------------------------------------------------------------------

fn_name Editor_AdjustLineIndentation = "Editor.AdjustLineIndentation";

word Editor::AdjustLineIndentation(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_AdjustLineIndentation);

   auto s = FindLine(log.line, expl);
   if(s == source_.end()) return 0;
   Indent(s, false);
   return Changed(s, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_AdjustTags = "Editor.AdjustTags";

word Editor::AdjustTags(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_AdjustTags);

   auto tag = (log.warning == PtrTagDetached ? '*' : '&');
   string target = "Detached ";
   target.push_back(tag);
   target.push_back(SPACE);

   auto loc = FindPos(log.pos);
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

fn_name Editor_AlignArgumentNames = "Editor.AlignArgumentNames";

word Editor::AlignArgumentNames(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_AlignArgumentNames);

   //  This is logged on a definition when it uses a different argument name
   //  than the declaration, and on both when they use a different name than
   //  the base class (for an override).
   //
   auto arg = static_cast< const Argument* >(log.item);
   auto defn = static_cast< const Function* >(arg->GetScope());
   const Function* decl = nullptr;

   if(log.warning == DefinitionRenamesArgument)
      decl = defn->GetDecl();
   else
      decl = defn->GetBase();
   if(decl == nullptr) return NotFound(expl, "Function's declaration");

   //  Find the argument names used in the definition and the declaration.
   //
   auto& defnArgs = defn->GetArgs();
   if(log.offset >= defnArgs.size()) return NotFound(expl, "Argument");
   auto defnName = defnArgs.at(log.offset)->Name();

   auto& declArgs = decl->GetArgs();
   if(log.offset >= declArgs.size())
      return NotFound(expl, "Argument declaration");
   auto declName = declArgs.at(log.offset)->Name();

   //  Find the lines on which the function's code begins and ends.  Between
   //  those lines, replace instances of the definition's argument name with
   //  the declaration's name.
   //
   size_t first, right;
   defn->GetRange(first, right);
   if(right == string::npos) return NotFound(expl, "Right brace");
   auto begin = FindPos(log.item->GetPos());
   if(begin.pos == string::npos) return NotFound(expl, "Function name");
   auto lastLine = log.file->GetLexer().GetLineNum(right);
   auto end = FindLine(lastLine);
   if(end == source_.end()) return NotFound(expl, "Function last line");
   auto range = lastLine - log.line + 1;

   for(auto loc = FindWord(begin.iter, begin.pos, *defnName, &range);
       loc.pos != string::npos;
       loc = FindWord(loc.iter, loc.pos + 1, *defnName, &range))
   {
      loc.iter->code.erase(loc.pos, defnName->size());
      loc.iter->code.insert(loc.pos, *declName);
      Changed();
   }

   return Changed(begin.iter, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_ChangeClassToStruct = "Editor.ChangeClassToStruct";

word Editor::ChangeClassToStruct(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_ChangeClassToStruct);

   //  Look for the class's name and then back up to "class".
   //
   auto cls = FindPos(log.item->GetPos());
   if(cls.pos == string::npos) return NotFound(expl, "Class name");
   cls = Rfind(cls.iter, CLASS_STR, cls.pos);
   if(cls.pos == string::npos) return NotFound(expl, CLASS_STR, true);
   cls.iter->code.erase(cls.pos, strlen(CLASS_STR));
   cls.iter->code.insert(cls.pos, STRUCT_STR);

   //  If the class began with a "public:" access control, erase it.
   //
   auto left = Find(cls.iter, "{");
   if(left.pos == string::npos) return NotFound(expl, "Left brace");
   left = NextPos(left);
   auto acc = FindWord(left.iter, left.pos, PUBLIC_STR);
   if(acc.pos != string::npos)
   {
      auto colon = FindNonBlank(acc.iter, acc.pos + strlen(PUBLIC_STR));
      colon.iter->code.erase(colon.pos, 1);
      acc.iter->code.erase(acc.pos, strlen(PUBLIC_STR));
      if(acc.iter->code.empty()) source_.erase(acc.iter);
   }
   return Changed(cls.iter, expl);
}

//------------------------------------------------------------------------------

word Editor::Changed()
{
   changed_ = true;
   return 0;
}

//------------------------------------------------------------------------------

word Editor::Changed(const Iter& iter, string& expl)
{
   expl = (iter->code.empty() ? EMPTY_STR : iter->code);
   changed_ = true;
   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_ChangeDebugFtName = "Editor.ChangeDebugFtName";

word Editor::ChangeDebugFtName(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_ChangeDebugFtName);

   //  Find the locations of the existing fn_name definition and
   //  Debug::ft invocation.  The definition could span two lines.
   //
   auto func = FindPos(log.item->GetPos());
   if(func.pos == string::npos) return NotFound(expl, "Function name");
   auto dbegin = Rfind(func.iter, "fn_name", func.pos);
   if(dbegin.pos == string::npos)
      return NotFound(expl, "fn_name definition");
   auto dend = Find(dbegin.iter, ";", dbegin.pos);
   if(dend.pos == string::npos)
      return NotFound(expl, "fn_name semicolon");
   auto split = (dbegin.iter != dend.iter);
   auto cloc = Find(func.iter, "Debug::ft", func.pos);
   if(cloc.pos == string::npos)
      return NotFound(expl, "Debug::ft invocation");

   //  Get the replacement code for the definition and invocation.  Erase
   //  the current ones and replace them, splitting the definition if it
   //  is too long.
   //
   string defn, call;
   DebugFtCode(static_cast< const Function* >(log.item), defn, call);
   auto where = source_.erase(dbegin.iter);
   if(split) where = source_.erase(where);
   where = source_.insert(where, SourceLine(defn, SIZE_MAX));
   if(defn.size() > LINE_LENGTH_MAX)
   {
      auto pos = defn.find('=');
      InsertLineBreak(where, pos + 1);
   }
   where = source_.erase(cloc.iter);
   source_.insert(where, SourceLine(call, SIZE_MAX));
   return Changed(where, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_ChangeStructToClass = "Editor.ChangeStructToClass";

word Editor::ChangeStructToClass(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_ChangeStructToClass);

   //  Look for the struct's name and then back up to "struct".
   //
   auto str = FindPos(log.item->GetPos());
   if(str.pos == string::npos) return NotFound(expl, "Struct name");
   str = Rfind(str.iter, STRUCT_STR, str.pos);
   if(str.pos == string::npos) return NotFound(expl, STRUCT_STR, true);
   str.iter->code.erase(str.pos, strlen(STRUCT_STR));
   str.iter->code.insert(str.pos, CLASS_STR);

   //  Unless the struct began with a "public:" access control, insert one.
   //
   auto left = Find(str.iter, "{");
   if(left.pos == string::npos) return NotFound(expl, "Left brace");
   left = NextPos(left);
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

Editor::Iter Editor::CodeBegin()
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

   auto s = source_.end();

   if(first != SIZE_MAX)
   {
      auto line = file_->GetLexer().GetLineNum(first);
      s = FindLine(line);
   }

   auto ns = false;

   for(--s; s != source_.begin(); --s)
   {
      auto type = GetLineType(s);

      if(!LineTypeAttr::Attrs[type].isCode && (type != LeadingComment))
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
         //  This should be the the brace for a namespace enclosure.
         //
         ns = true;
         break;

      case Code:
         //
         //  If we saw an open brace, this should be a namespace enclosure.
         //  Generate a log if it's something else, otherwise continue to
         //  back up.  If a namespace wasn't expected, this is probably a
         //  forward declaration, so assume that the code starts after it.
         //
         if(ns)
         {
            if(s->code.find(NAMESPACE_STR) != string::npos) continue;
            Debug::SwLog(Editor_CodeBegin, "Namespace expected", s->line + 1);
         }
         return ++s;

      case DebugFt:
      case FunctionName:
      case FunctionNameSplit:
         //
         //  These shouldn't occur.
         //
         Debug::SwLog(Editor_CodeBegin, "Unexpected line type", type);
         // [[fallthrough]]
      case LeadingComment:
      case CloseBrace:
      case CloseBraceSemicolon:
      case IncludeDirective:
      case HashDirective:
      case UsingDirective:
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

fn_name Editor_ConvertTabsToBlanks = "Editor.ConvertTabsToBlanks";

word Editor::ConvertTabsToBlanks()
{
   Debug::ft(Editor_ConvertTabsToBlanks);

   //  Run through the source, looking for a line of code that contains a tab.
   //
   for(auto s = source_.begin(); s != source_.end(); ++s)
   {
      auto pos = s->code.find_first_of(TAB);

      if(pos != string::npos)
      {
         //  A tab has been found.  Copy the characters that precede it
         //  into UNTABBED.  Run through the rest of the line, copying
         //  non-tab characters to UNTABBED.  Replace each tab with the
         //  number of spaces needed to reach the next tab stop, namely
         //  the next even multiple of INDENT_SIZE.  This code doesn't
         //  bother to skip over character and string literals, which
         //  should use \t or TAB or something that is actually visible.
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
               int spaces = untabbed.size() % INDENT_SIZE;
               if(spaces == 0) spaces = INDENT_SIZE;

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

fn_name Editor_DebugFtCode = "Editor.DebugFtCode";

void Editor::DebugFtCode
   (const Function* func, std::string& defn, std::string& call)
{
   Debug::ft(Editor_DebugFtCode);

   //  Assemble the following:
   //  o fname: the function's name
   //  o sname: the name of the scope in which the function appears
   //  o dslit: the string literal that identifies the function
   //  o dname: the name for the fn_name string literal
   //
   auto fname = func->DebugName();
   auto sname = *func->GetScope()->Name();

   string dslit = QUOTE + sname;
   if(!sname.empty()) dslit.push_back('.');
   dslit.append(fname);
   dslit.push_back(QUOTE);

   auto dname = sname;
   if(!sname.empty()) dname.push_back('_');
   if(func->FuncType() == FuncOperator)
   {
      //  Something like "class_operator=" won't pass as an identifier, so use
      //  "class_operatorN", where N is the integer value of the Operator enum.
      //
      auto oper = CxxOp::NameToOperator(fname);
      fname.erase(strlen(OPERATOR_STR));
      fname.append(std::to_string(oper));
   }
   dname.append(fname);
   defn = "fn_name";
   defn.push_back(SPACE);
   defn.append(dname);
   defn.append(" = ");
   defn.append(dslit);
   defn.push_back(';');
   call = string(INDENT_SIZE, SPACE) + "Debug::ft(";
   call.append(dname);
   call.append(");");
}

//------------------------------------------------------------------------------

fn_name Editor_DeleteLineBreak = "Editor.DeleteLineBreak";

bool Editor::DeleteLineBreak(const Iter& iter)
{
   Debug::ft(Editor_DeleteLineBreak);

   auto next = std::next(iter);
   if(next == source_.end()) return false;

   //  Check that the lines can be merged.
   //
   auto type = GetLineType(iter);
   if(!LineTypeAttr::Attrs[type].isMergeable) return false;
   type = GetLineType(next);
   if(!LineTypeAttr::Attrs[type].isMergeable) return false;
   if(iter->code.find(COMMENT_STR) != string::npos) return false;
   if(iter->code.find(';') != string::npos) return false;

   //  Check that the merged line wouldn't be too long.
   //
   auto begin2 = next->code.find_first_not_of(WhitespaceChars);
   auto size1 = iter->code.size();
   auto size2 = next->code.size() - begin2;
   if(size1 + size2 > LINE_LENGTH_MAX) return false;

   //  Merge the lines.
   //
   next->code.erase(0, begin2);
   iter->code.append(next->code);
   source_.erase(next);
   return true;
}

//------------------------------------------------------------------------------

fn_name Editor_DemangleInclude = "Editor.DemangleInclude";

string Editor::DemangleInclude(string code) const
{
   Debug::ft(Editor_DemangleInclude);

   if(code.empty()) return code;
   if(code.find(HASH_INCLUDE_STR) != 0) return code;

   auto pos = code.find_first_of(FrontChars);
   if(pos == string::npos) return code;

   switch(FrontChars.find_first_of(code[pos]))
   {
   case 0:
   case 2:
      code[pos] = '<';
      code.back() = '>';
      break;
   case 1:
   case 3:
      code[pos] = QUOTE;
      code.back() = QUOTE;
      break;
   }

   return code;
}

//------------------------------------------------------------------------------

fn_name Editor_DisplayLog = "Editor.DisplayLog";

void Editor::DisplayLog(const CliThread& cli, const WarningLog& log, bool file)
{
   Debug::ft(Editor_DisplayLog);

   if(file)
   {
      *cli.obuf << log.file->Name() << ':' << CRLF;
   }

   //  Display LOG's details.
   //
   *cli.obuf << spaces(2) << "Line " << log.line + 1;
   if(log.offset != 0) *cli.obuf << '/' << log.offset;
   *cli.obuf << ": " << Warning(log.warning);
   if(log.DisplayInfo()) *cli.obuf << ": " << log.info;
   *cli.obuf << CRLF;

   if(log.DisplayCode())
   {
      //  Display the current version of the code associated with LOG.
      //
      auto s = FindLine(log.line);

      if(s == source_.end())
      {
         *cli.obuf << "Line " << log.line + 1 << " not found." << CRLF;
         return;
      }

      *cli.obuf << spaces(2);
      *cli.obuf << DemangleInclude(s->code) << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name Editor_EraseAccessControl = "Editor.EraseAccessControl";

word Editor::EraseAccessControl(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_EraseAccessControl);

   //  The parser logs RedundantAccessControl at the position
   //  where it occurred; log.item is nullptr in this warning.
   //
   auto s = FindLine(log.line, expl);
   if(s == source_.end()) return 0;
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

fn_name Editor_EraseAdjacentSpaces = "Editor.EraseAdjacentSpaces";

word Editor::EraseAdjacentSpaces(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_EraseAdjacentSpaces);

   auto s = FindLine(log.line, expl);
   if(s == source_.end()) return 0;

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
      if(s != source_.begin())
      {
         auto prev = std::prev(s);
         move = (comm == prev->code.find_first_of("//"));
      }

      if(!move)
      {
         auto next = std::next(s);
         if(next != source_.end())
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
      stop = code.size();

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

fn_name Editor_EraseBlankLine = "Editor.EraseBlankLine";

word Editor::EraseBlankLine(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_EraseBlankLine);

   //  Remove the specified line of code.
   //
   auto blank = FindPos(log.pos);
   if(blank.pos == string::npos) return NotFound(expl, "Blank line");
   source_.erase(blank.iter);
   return Changed();
}

//------------------------------------------------------------------------------

fn_name Editor_EraseBlankLinePairs = "Editor.EraseBlankLinePairs";

word Editor::EraseBlankLinePairs()
{
   Debug::ft(Editor_EraseBlankLinePairs);

   auto i1 = source_.begin();
   if(i1 == source_.end()) return 0;

   for(auto i2 = std::next(i1); i2 != source_.end(); i2 = std::next(i1))
   {
      if(i1->code.empty() && i2->code.empty())
      {
         i1 = source_.erase(i1);
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

fn_name Editor_EraseConst = "Editor.EraseConst";

word Editor::EraseConst(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_EraseConst);

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
   auto tag = FindPos(log.pos);
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

fn_name Editor_EraseEmptyNamespace = "Editor.EraseEmptyNamespace";

word Editor::EraseEmptyNamespace(const Iter& iter)
{
   Debug::ft(Editor_EraseEmptyNamespace);

   //  ITER references the line that follows a forward declaration which was
   //  just deleted.  If this left an empty "namespace <ns> { }", remove it.
   //
   if(iter == source_.end()) return 0;
   if(iter->code.find('}') != 0) return 0;

   auto up1 = std::prev(iter);
   if(up1 == source_.begin()) return 0;
   auto up2 = std::prev(up1);

   if((up2->code.find(NAMESPACE_STR) == 0) && (up1->code.find('{') == 0))
   {
      source_.erase(up2, std::next(iter));
      return Changed();
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_EraseForward = "Editor.EraseForward";

word Editor::EraseForward(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_EraseForward);

   //  Erase the line where the forward declaration appears.  This
   //  may leave an empty enclosing namespace that should be deleted.
   //
   auto forw = FindPos(log.pos);
   if(forw.pos == string::npos)
      return NotFound(expl, "Forward declaration");
   forw.iter = source_.erase(forw.iter);
   Changed();
   return EraseEmptyNamespace(forw.iter);
}

//------------------------------------------------------------------------------

fn_name Editor_EraseInclude = "Editor.EraseInclude";

word Editor::EraseInclude(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_EraseInclude);

   //  Remove the specified line of code.
   //
   auto incl = FindPos(log.pos);
   if(incl.pos == string::npos) return NotFound(expl, HASH_INCLUDE_STR);
   source_.erase(incl.iter);
   return Changed();
}

//------------------------------------------------------------------------------

fn_name Editor_EraseSemicolon = "Editor.EraseSemicolon";

word Editor::EraseSemicolon(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_EraseSemicolon);

   //  The parser logs a redundant semicolon that follows the closing '}'
   //  of a function definition or namespace enclosure.
   //
   auto s = FindLine(log.line, expl);
   if(s == source_.end()) return 0;
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

fn_name Editor_EraseTrailingBlanks = "Editor.EraseTrailingBlanks";

word Editor::EraseTrailingBlanks()
{
   Debug::ft(Editor_EraseTrailingBlanks);

   for(auto s = source_.begin(); s != source_.end(); ++s)
   {
      while(!s->code.empty() && IsBlank(s->code.back()))
      {
         s->code.pop_back();
         Changed();
      }
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_EraseUsing = "Editor.EraseUsing";

word Editor::EraseUsing(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_EraseUsing);

   //  Erase the line where the using statement appears.
   //
   auto use = FindPos(log.pos);
   if(use.pos == string::npos)
      return NotFound(expl, "Using declaration");
   source_.erase(use.iter);
   return Changed();
}

//------------------------------------------------------------------------------

fn_name Editor_EraseVoidArgument = "Editor.EraseVoidArgument";

word Editor::EraseVoidArgument(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_EraseVoidArgument);

   //  The function might return "void", so the second occurrence of "void"
   //  could be the argument.  Erase it, leaving only the parentheses.
   //
   auto s = FindLine(log.line, expl);
   if(s == source_.end()) return 0;

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

Editor::CodeLocation Editor::Find(Iter iter, const string& str, size_t off)
{
   while(iter != source_.end())
   {
      auto pos = iter->code.find(str, off);
      if(pos != string::npos) return CodeLocation(iter, pos);
      ++iter;
      off = 0;
   }

   return CodeLocation(iter);
}

//------------------------------------------------------------------------------

Editor::CodeLocation Editor::FindFirstOf
   (Iter iter, size_t off, const string& chars)
{
   while(iter != source_.end())
   {
      auto pos = iter->code.find_first_of(chars, off);
      if(pos != string::npos) return CodeLocation(iter, pos);
      ++iter;
      off = 0;
   }

   return CodeLocation(iter);
}

//------------------------------------------------------------------------------

Editor::Iter Editor::FindLine(size_t line)
{
   for(auto s = source_.begin(); s != source_.end(); ++s)
   {
      if(s->line == line) return s;
   }

   return source_.end();
}

//------------------------------------------------------------------------------

Editor::Iter Editor::FindLine(size_t line, string& expl)
{
   auto s = FindLine(line);

   if(s == source_.end())
   {
      expl = "Line " + std::to_string(line + 1) + " not found.";
   }

   return s;
}

//------------------------------------------------------------------------------

fn_name Editor_FindNonBlank = "Editor.FindNonBlank";

Editor::CodeLocation Editor::FindNonBlank(Iter iter, size_t pos)
{
   Debug::ft(Editor_FindNonBlank);

   while(iter != source_.end())
   {
      for(NO_OP; pos < iter->code.size(); ++pos)
      {
         if(!IsBlank(iter->code[pos])) return CodeLocation(iter, pos);
      }

      ++iter;
      pos = 0;
   }

   return CodeLocation(iter);
}

//------------------------------------------------------------------------------

fn_name Editor_FindPos = "Editor.FindPos";

Editor::CodeLocation Editor::FindPos(size_t pos)
{
   Debug::ft(Editor_FindPos);

   //  Find ITEM's location within its file, and convert this to a line
   //  number and offset on that line.
   //
   auto& lexer = file_->GetLexer();
   auto line = lexer.GetLineNum(pos);
   if(line == string::npos) return CodeLocation(source_.end());
   auto iter = FindLine(line);
   if(iter == source_.end()) return CodeLocation(iter);
   auto start = lexer.GetLineStart(line);
   return CodeLocation(iter, pos - start);
}

//------------------------------------------------------------------------------

fn_name Editor_FindUsingReferents = "Editor.FindUsingReferents";

CxxNamedSet Editor::FindUsingReferents(const CxxNamed* item) const
{
   Debug::ft(Editor_FindUsingReferents);

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

fn_name Editor_FindWord = "Editor.FindWord";

Editor::CodeLocation Editor::FindWord
   (Iter iter, size_t pos, const string& id, size_t* range)
{
   Debug::ft(Editor_FindWord);

   //  Look for ID, which must be preceded and followed by characters
   //  that are not allowed in an identifier.
   //
   while(iter != source_.end())
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
            if(prev && next) return CodeLocation(iter, pos);
            pos = iter->code.find(id, pos + 1);
         }
      }

      if((range == nullptr) || (--*range == 0))
         return CodeLocation(source_.end());
      ++iter;
      pos = 0;
   }

   return CodeLocation(source_.end());
}

//------------------------------------------------------------------------------

fixed_string FixPrompt = "  Fix?";
fixed_string FixChars = "ynsq";
fixed_string FixHelp = "Enter y(yes) n(no) s(skip file) q(quit): ";

fn_name Editor_Fix = "Editor.Fix";

word Editor::Fix(CliThread& cli, string& expl)
{
   Debug::ft(Editor_Fix);

   //  Get the file's warnings and source code.
   //
   auto rc = Read(expl);
   if(rc != 0) return rc;

   //  Run through all the warnings.  If fixing a warning is not supported,
   //  skip it.
   //
   auto found = false;
   auto first = true;
   char reply = 'y';
   auto exit = false;

   for(auto item = warnings_.begin(); item != warnings_.end(); ++item)
   {
      switch(FixStatus(*item))
      {
      case NotFixed:
         found = true;
         break;
      case Pending:
         found = true;
         //  [[fallthrough]]
      case Fixed:
         continue;
      }

      DisplayLog(cli, *item, first);
      first = false;

      //  See if the user wishes to fix the warning.  The valid responses are
      //  o 'y' = fix it, which invokes the appropriate function
      //  o 'n' = don't fix it
      //  o 's' = skip this file
      //  o 'q' = done fixing warnings
      //
      expl.clear();
      reply = cli.CharPrompt(FixPrompt, FixChars, FixHelp);

      switch(reply)
      {
      case 'y':
         rc = FixWarning(*item, expl);
         if(rc == 0) (item)->status = Pending;

         //  Display EXPL if it isn't empty, else assume that the fix succeeded.
         //
         *cli.obuf << spaces(2) << (expl.empty() ? SuccessExpl : expl) << CRLF;
         break;

      case 'n':
         break;

      case 's':
      case 'q':
         exit = true;
      }

      cli.Flush();
      if(exit || (rc != 0)) break;
   }

   if(found && (rc == 0))
   {
      *cli.obuf << spaces(2) << "End of supported warnings." << CRLF;
   }

   //  Write out the file if it was opened.  The possible outcomes are
   //    < 0: an error occurred.
   //    = 0: the file wasn't changed.
   //    = 1: the file was changed.  Inform the user.
   //  If the user entered 'q', return -1 to stop fixing files unless an
   //  error occurred.
   //
   rc = Write(file_->FullName(), expl);

   if(rc == 1)
   {
      *cli.obuf << spaces(2) << "...committed." << CRLF;

      for(auto item = warnings_.begin(); item != warnings_.end(); ++item)
      {
         if(item->status == Pending) item->status = Fixed;
      }
   }

   if(rc >= 0) rc = (reply == 'q' ? -2 : 0);

   //  A result of -1 or greater indicates that the next file can still be
   //  processed, so don't report an error value.
   //
   return (rc < -1 ? rc : 0);
}

//------------------------------------------------------------------------------

fn_name Editor_FixStatus = "Editor.FixStatus";

WarningStatus Editor::FixStatus(const WarningLog& log) const
{
   Debug::ft(Editor_FixStatus);

   switch(log.status)
   {
   case Pending:
      return Pending;
   case Fixed:
      return Fixed;
   }

   switch(log.warning)
   {
   case IncludeNotSorted:
      if(sorted_) return Fixed;
      //  [[fallthrough]]
   case UseOfSlashAsterisk:
   case UseOfNull:
   case PtrTagDetached:
   case RefTagDetached:
   case RedundantSemicolon:
   case RedundantConst:
   case IncludeGuardMissing:
   case IncludeDuplicated:
   case IncludeAdd:
   case IncludeRemove:
   case HeaderReliesOnUsing:
   case UsingInHeader:
   case UsingDuplicated:
   case UsingAdd:
   case UsingRemove:
   case ForwardAdd:
   case ForwardRemove:
   case ClassCouldBeStruct:
   case StructCouldBeClass:
   case RedundantAccessControl:
   case DataCouldBeConst:
   case DataCouldBeConstPtr:
   case DataNeedNotBeMutable:
   case NonExplicitConstructor:
   case VirtualTagMissing:
   case OverrideTagMissing:
   case VoidAsArgument:
   case DefinitionRenamesArgument:
   case OverrideRenamesArgument:
   case ArgumentCouldBeConstRef:
   case ArgumentCouldBeConst:
   case FunctionCouldBeConst:
   case FunctionCouldBeStatic:
   case UseOfTab:
   case Indentation:
   case TrailingSpace:
   case AdjacentSpaces:
   case AddBlankLine:
   case RemoveBlankLine:
//c   case LineLength:
   case IncludeGuardMisnamed:
   case DebugFtNotInvoked:
   case DebugFtNameMismatch:
   case FunctionCouldBeDefaulted:
   case InitCouldUseConstructor:
      return NotFixed;
   }

   return Fixed;
}

//------------------------------------------------------------------------------

fn_name Editor_FixWarning = "Editor.FixWarning";

word Editor::FixWarning(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_FixWarning);

   switch(log.warning)
   {
   case UseOfSlashAsterisk:
      return ReplaceSlashAsterisk(log, expl);
   case UseOfNull:
      return ReplaceNull(log, expl);
   case PtrTagDetached:
   case RefTagDetached:
      return AdjustTags(log, expl);
   case RedundantSemicolon:
      return EraseSemicolon(log, expl);
   case RedundantConst:
      return EraseConst(log, expl);
   case IncludeGuardMissing:
      return InsertIncludeGuard(log, expl);
   case IncludeNotSorted:
      return SortIncludes(expl);
   case IncludeAdd:
      return InsertInclude(log, expl);
   case IncludeDuplicated:
   case IncludeRemove:
      return EraseInclude(log, expl);
   case HeaderReliesOnUsing:
      return ResolveUsings();
   case UsingInHeader:
      return ReplaceUsing(log, expl);
   case UsingAdd:
      return InsertUsing(log, expl);
   case UsingDuplicated:
   case UsingRemove:
      return EraseUsing(log, expl);
   case ForwardAdd:
      return InsertForward(log, expl);
   case ForwardRemove:
      return EraseForward(log, expl);
   case ClassCouldBeStruct:
      return ChangeClassToStruct(log, expl);
   case StructCouldBeClass:
      return ChangeStructToClass(log, expl);
   case RedundantAccessControl:
      return EraseAccessControl(log, expl);
   case DataCouldBeConst:
      return TagAsConstData(log, expl);
   case DataCouldBeConstPtr:
      return TagAsConstPointer(log, expl);
   case DataNeedNotBeMutable:
      return UntagAsMutable(log, expl);
   case NonExplicitConstructor:
      return TagAsExplicit(log, expl);
   case VirtualTagMissing:
      return TagAsVirtual(log, expl);
   case OverrideTagMissing:
      return TagAsOverride(log, expl);
   case VoidAsArgument:
      return EraseVoidArgument(log, expl);
   case DefinitionRenamesArgument:
   case OverrideRenamesArgument:
      return AlignArgumentNames(log, expl);
   case ArgumentCouldBeConstRef:
      return TagAsConstReference(log, expl);
   case ArgumentCouldBeConst:
      return TagAsConstArgument(log, expl);
   case FunctionCouldBeConst:
      return TagAsConstFunction(log, expl);
   case FunctionCouldBeStatic:
      return TagAsStaticFunction(log, expl);
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
   case IncludeGuardMisnamed:
      return RenameIncludeGuard(log, expl);
   case DebugFtNotInvoked:
      return InsertDebugFtCall(log, expl);
   case DebugFtNameMismatch:
      return ChangeDebugFtName(log, expl);
   case FunctionCouldBeDefaulted:
      return TagAsDefaulted(log, expl);
   case InitCouldUseConstructor:
      return InitByConstructor(log, expl);
   default:
      expl = "Fixing this type of warning is not supported.";
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_Format = "Editor.Format";

word Editor::Format(string& expl)
{
   Debug::ft(Editor_Format);

   auto rc = Read(expl);
   if(rc != 0) return rc;

   EraseTrailingBlanks();
   EraseBlankLinePairs();
   ConvertTabsToBlanks();
   rc = Write(file_->FullName(), expl);
   return (rc >= 0 ? 0 : rc);
}

//------------------------------------------------------------------------------

fn_name Editor_GetCode = "Editor.GetCode";

word Editor::GetCode(string& expl)
{
   Debug::ft(Editor_GetCode);

   string str;

   //  Read each line of code and save it.  Before it is saved, an
   //  #include directive is mangled for sorting purposes.
   //
   while(input_->peek() != EOF)
   {
      std::getline(*input_, str);

      if(str.find(HASH_INCLUDE_STR) == 0)
      {
         auto rc = MangleInclude(str, expl);
         if(rc != 0) return rc;
      }

      PushBack(str);
   }

   return 0;
}

//------------------------------------------------------------------------------

LineType Editor::GetLineType(const Iter& iter) const
{
   std::set< Warning > warnings;

   if(iter->line != SIZE_MAX) return file_->GetLineType(iter->line);
   return CodeFile::ClassifyLine(iter->code, warnings);
}

//------------------------------------------------------------------------------

fn_name Editor_IncludesBegin = "Editor.IncludesBegin";

Editor::Iter Editor::IncludesBegin()
{
   Debug::ft(Editor_IncludesBegin);

   for(auto s = source_.begin(); s != source_.end(); ++s)
   {
      if(s->code.find(HASH_INCLUDE_STR) == 0) return s;
   }

   return source_.end();
}

//------------------------------------------------------------------------------

fn_name Editor_IncludesEnd = "Editor.IncludesEnd";

Editor::Iter Editor::IncludesEnd()
{
   Debug::ft(Editor_IncludesEnd);

   for(auto s = IncludesBegin(); s != source_.end(); ++s)
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

   return source_.end();
}

//------------------------------------------------------------------------------

fn_name Editor_Indent = "Editor.Indent";

size_t Editor::Indent(const Iter& iter, bool split)
{
   Debug::ft(Editor_Indent);

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
      if(curr == source_.begin())
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

   auto indent = depth * INDENT_SIZE;
   iter->code.insert(0, indent, SPACE);
   Changed();
   return indent;
}

//------------------------------------------------------------------------------

fn_name Editor_InitByConstructor = "Editor.InitByConstructor";

word Editor::InitByConstructor(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_InitByConstructor);

   //  Change ["const"] <type> <name> = <class> "(" [<args>] ");"
   //      to ["const"] <class> <name> "(" <args> ");"
   //
   auto loc = FindPos(log.pos);
   if(loc.pos == string::npos) return NotFound(expl, "Statement");
   auto name = FindWord(loc.iter, 0, *log.item->Name());
   if(name.pos == string::npos) return NotFound(expl, "Variable name");
   auto vprev = name.iter->code.rfind(SPACE, name.pos);
   if(vprev == string::npos) return NotFound(expl, "Start of variable name");
   auto eq = FindFirstOf(name.iter, name.pos, "=");
   if(eq.pos == string::npos) return NotFound(expl, "Assignment operator");
   auto vend = RfindNonBlank(eq.iter, eq.pos - 1);
   if(vend.pos == string::npos) return NotFound(expl, "End of variable name");
   auto lpar = FindFirstOf(eq.iter, eq.pos, "(");
   if(lpar.pos == string::npos) return NotFound(expl, "Left parenthesis");
   auto semi = FindFirstOf(eq.iter, eq.pos, ";");
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
   if(log.item->IsConst()) code.insert(0, "const ");

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
      DeleteLineBreak(name.iter);
   }

   return Changed(name.iter, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_Insert = "Editor.Insert";

Editor::Iter Editor::Insert(Iter iter, const string& code)
{
   Debug::ft(Editor_Insert);

   Changed();
   return source_.insert(iter, SourceLine(code, SIZE_MAX));
}

//------------------------------------------------------------------------------

fn_name Editor_InsertBlankLine = "Editor.InsertBlankLine";

word Editor::InsertBlankLine(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_InsertBlankLine);

   auto s = FindLine(log.line, expl);
   if(s == source_.end()) return 0;
   Insert(s, EMPTY_STR);
   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_InsertDebugFtCall = "Editor.InsertDebugFtCall";

word Editor::InsertDebugFtCall(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_InsertDebugFtCall);

   auto name = FindPos(log.item->GetPos());
   if(name.pos == string::npos) return NotFound(expl, "Function name");
   auto left = FindFirstOf(name.iter, name.pos, "{");
   if(left.pos == string::npos) return NotFound(expl, "Left brace");

   //  Get the code for the fn_name declaration and Debug::ft invocation.
   //  EXTRA gets set if anything follows the left brace on the same line.
   //
   string defn, call;
   DebugFtCode(static_cast< const Function* >(log.item), defn, call);
   auto extra = (left.iter->code.find_first_not_of
      (WhitespaceChars, left.pos + 1) != string::npos);

   //  Check if the fn_name was already defined and the Debug::ft call
   //  simply forgotten.
   //
   auto fn = defn;
   fn.erase(0, strlen("fn_name "));
   auto apos = fn.find('=');
   fn.erase(apos - 1);
   auto curr = Rfind(name.iter, fn, name.pos);

   if(curr.iter == source_.end())
   {
      //  Insert DEFN and a blank line above the function.
      //
      auto above = Insert(name.iter, EMPTY_STR);
      above = Insert(above, defn);

      if(defn.size() > LINE_LENGTH_MAX)
      {
         auto pos = defn.find('=');
         InsertLineBreak(above, pos + 1);
      }
   }

   //  Insert CALL and a blank line after the left brace.  If the left brace
   //  wasn't on a new line, push it down first.  If something followed the
   //  left brace, push it down as well.
   //
   if(left.iter->code.find_first_not_of(WhitespaceChars) != left.pos)
      left = InsertLineBreak(left.iter, left.pos);
   if(extra) InsertLineBreak(left.iter, left.pos + 1);
   auto below = std::next(left.iter);
   below = Insert(below, EMPTY_STR);
   below = Insert(below, call);
   return Changed(below, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_InsertForward1 = "Editor.InsertForward(log)";

word Editor::InsertForward(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_InsertForward1);

   //  LOG must provide the namespace for the forward declaration.
   //
   auto qname = log.info;
   auto pos = qname.find(SCOPE_STR);
   if(pos == string::npos)
      return Report(expl, "Symbol's namespace not provided.");

   //  LOG must also specify whether the forward declaration is for a class,
   //  struct, or union (unlikely).
   //
   string areaStr;

   if(qname.find(CLASS_STR) == 0)
      areaStr = CLASS_STR;
   else if(qname.find(STRUCT_STR) == 0)
      areaStr = STRUCT_STR;
   else if(qname.find(UNION_STR) == 0)
      areaStr = UNION_STR;
   else
      return Report(expl, "Symbol must specify \"class\" or \"struct\".");

   string forward = spaces(INDENT_SIZE);
   forward += areaStr;
   forward.push_back(SPACE);
   forward += qname.substr(pos + 2);
   forward.push_back(';');

   //  Set NSPACE to "namespace <ns>", where <ns> is the symbol's namespace.
   //  Then decide where to insert the forward declaration.
   //
   string nspace = NAMESPACE_STR;
   nspace += qname.substr(areaStr.size(), pos - areaStr.size());
   auto code = CodeBegin();

   for(auto s = PrologEnd(); s != source_.end(); ++s)
   {
      if(s->code.find(NAMESPACE_STR) == 0)
      {
         //  If this namespace matches NSPACE, add the declaration to it.
         //  If this namespace's name is alphabetically after NSPACE, add
         //  the declaration before it, along with its namespace.
         //
         auto comp = strCompare(s->code, nspace);
         if(comp == 0) return InsertForward(s, forward, expl);
         if(comp > 0) return InsertNamespaceForward(s, nspace, forward);
      }
      else if((s->code.find(USING_STR) == 0) || (s == code))
      {
         //  We have now passed any existing forward declarations, so add
         //  the new declaration here, along with its namespace.
         //
         return InsertNamespaceForward(s, nspace, forward);
      }
   }

   return Report(expl, "Failed to insert forward declaration.");
}

//------------------------------------------------------------------------------

fn_name Editor_InsertForward2 = "Editor.InsertForward(iter)";

word Editor::InsertForward
   (const Iter& iter, const string& forward, string& expl)
{
   Debug::ft(Editor_InsertForward2);

   //  ITER references a namespace that matches the one for a new forward
   //  declaration.  Insert the new declaration alphabetically within the
   //  declarations that already appear in this namespace.
   //
   for(auto s = std::next(iter, 2); s != source_.end(); ++s)
   {
      if((strCompare(s->code, forward) > 0) ||
         (s->code.find('}') != string::npos))
      {
         s = Insert(s, forward);
         return Changed(s, expl);
      }
   }

   return Report(expl, "Failed to insert forward declaration.");
}

//------------------------------------------------------------------------------

fn_name Editor_InsertInclude1 = "Editor.InsertInclude(log)";

word Editor::InsertInclude(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_InsertInclude1);

   //  Create the new #include directive and add it to the list.
   //  Its file name appears in log.info.
   //
   string include = HASH_INCLUDE_STR;
   include.push_back(SPACE);
   include += log.info;
   return InsertInclude(include, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_InsertInclude2 = "Editor.InsertInclude(string)";

word Editor::InsertInclude(string& include, string& expl)
{
   Debug::ft(Editor_InsertInclude2);

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

      if(!IsSorted2(s->code, include))
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

fn_name Editor_InsertIncludeGuard = "Editor.InsertIncludeGuard";

word Editor::InsertIncludeGuard(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_InsertIncludeGuard);

   //  Insert the guard before the first #include.  If there isn't one,
   //  insert it before the first line of code.  If there isn't any code,
   //  insert it at the end of the file.
   //
   auto where = IncludesBegin();
   if(where == source_.end()) where = PrologEnd();
   string guardname = log.file->MakeGuardName();
   string code = "#define " + guardname;
   where = Insert(where, EMPTY_STR);
   where = Insert(where, code);
   code = "#ifndef " + guardname;
   where = Insert(where, code);
   Insert(source_.end(), HASH_ENDIF_STR);
   return Changed(where, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_InsertLineBreak1 = "Editor.InsertLineBreak(pos)";

Editor::CodeLocation Editor::InsertLineBreak(const Iter& iter, size_t pos)
{
   Debug::ft(Editor_InsertLineBreak1);

   if(iter->code.size() <= pos) return CodeLocation(source_.end());
   if(iter->code.find_first_not_of(WhitespaceChars, pos) == string::npos)
      return CodeLocation(iter, pos);
   auto code = iter->code.substr(pos);
   iter->code.erase(pos);
   auto next = Insert(std::next(iter), code);
   auto indent = Indent(next, true);
   return CodeLocation(next, indent);
}

//------------------------------------------------------------------------------

fn_name Editor_InsertLineBreak2 = "Editor.InsertLineBreak(log)";

word Editor::InsertLineBreak(const WarningLog& log, string& expl)  //c
{
   Debug::ft(Editor_InsertLineBreak2);

   auto s = FindLine(log.line, expl);
   if(s == source_.end()) return 0;
   return -1;
}

//------------------------------------------------------------------------------

fn_name Editor_InsertNamespaceForward = "Editor.InsertNamespaceForward";

word Editor::InsertNamespaceForward
   (const Iter& iter, const string& nspace, const string& forward)
{
   Debug::ft(Editor_InsertNamespaceForward);

   //  Insert a new forward declaration, along with an enclosing namespace,
   //  at ITER.  Offset it with blank lines.
   //
   auto s = Insert(iter, EMPTY_STR);
   s = Insert(s, "}");
   s = Insert(s, forward);
   s = Insert(s, "{");
   s = Insert(s, nspace);
   s = Insert(s, EMPTY_STR);
   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_InsertPrefix = "Editor.InsertPrefix";

void Editor::InsertPrefix(const Iter& iter, size_t pos, const string& prefix)
{
   Debug::ft(Editor_InsertPrefix);

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

fn_name Editor_InsertUsing = "Editor.InsertUsing";

word Editor::InsertUsing(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_InsertUsing);

   //  Create the new using statement and decide where to insert it.
   //
   string statement = USING_STR;
   statement.push_back(SPACE);
   statement += log.info;
   statement.push_back(';');

   auto code = CodeBegin();
   auto usings = false;

   for(auto s = PrologEnd(); s != source_.end(); ++s)
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
      else if((usings && s->code.empty()) || (s == code))
      {
         //  We have now passed any existing using statements, so add
         //  the new statement here.  If it is the first one, precede
         //  it with a blank line.
         //
         s = Insert(s, EMPTY_STR);
         s = Insert(s, statement);
         if(!usings) s = Insert(s, EMPTY_STR);
         return Changed(s, expl);
      }
   }

   return Report(expl, "Failed to insert using statement.");
}

//------------------------------------------------------------------------------

bool Editor::IsSorted1(const SourceLine& line1, const SourceLine& line2)
{
   return IsSorted2(line1.code, line2.code);
}

//------------------------------------------------------------------------------

bool Editor::IsSorted2(const string& line1, const string& line2)
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

fn_name Editor_MangleInclude = "Editor.MangleInclude";

word Editor::MangleInclude(string& include, string& expl) const
{
   Debug::ft(Editor_MangleInclude);

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

Editor::CodeLocation Editor::NextPos(const CodeLocation& curr)
{
   auto next = curr;

   while(++next.pos >= next.iter->code.size())
   {
      if(++next.iter == source_.end())
      {
         next.pos = string::npos;
         return next;
      }

      next.pos = string::npos;  // will wrap around to 0
   }

   return next;
}

//------------------------------------------------------------------------------

Editor::CodeLocation Editor::PrevPos(const CodeLocation& curr)
{
   auto prev = curr;

   if(--prev.pos != string::npos) return prev;  // continue if pos was 0

   if(prev.iter == source_.begin())
   {
      return CodeLocation(source_.end());
   }

   while(--prev.iter != source_.begin())
   {
      if(prev.iter->code.empty()) continue;
      prev.pos = prev.iter->code.size() - 1;
      return prev;
   }

   if(prev.iter->code.empty()) return CodeLocation(source_.end());
   prev.pos = prev.iter->code.size() - 1;
   return prev;
}

//------------------------------------------------------------------------------

fn_name Editor_PrologEnd = "Editor.PrologEnd";

Editor::Iter Editor::PrologEnd()
{
   Debug::ft(Editor_PrologEnd);

   for(auto s = source_.begin(); s != source_.end(); ++s)
   {
      if(LineTypeAttr::Attrs[GetLineType(s)].isCode) return s;
   }

   return source_.end();
}

//------------------------------------------------------------------------------

void Editor::PushBack(const string& code)
{
   source_.push_back(SourceLine(code, line_));
   ++line_;
}

//------------------------------------------------------------------------------

fn_name Editor_QualifyReferent = "Editor.QualifyReferent";

void Editor::QualifyReferent(const CxxNamed* item, const CxxNamed* ref)
{
   Debug::ft(Editor_QualifyReferent);

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
      if(s == source_.end()) continue;

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

fn_name Editor_QualifyUsings = "Editor.QualifyUsings";

void Editor::QualifyUsings(const CxxNamed* item)
{
   Debug::ft(Editor_QualifyUsings);

   auto refs = FindUsingReferents(item);

   for(auto r = refs.cbegin(); r != refs.cend(); ++r)
   {
      QualifyReferent(item, *r);
   }
}

//------------------------------------------------------------------------------

fn_name Editor_Read = "Editor.Read";

word Editor::Read(string& expl)
{
   Debug::ft(Editor_Read);

   if(read_ != 1) return read_;
   read_ = 0;

   //  Get the file's warnings and sort those on the same line by reverse
   //  position.  This reduces the chances of an item's position changing
   //  before it is edited.
   //
   CodeInfo::GetWarnings(file_, warnings_);
   if(warnings_.empty()) return -1;

   std::sort(warnings_.begin(), warnings_.end(), CodeInfo::IsSortedByLine);

   //  Get the file's source code.
   //
   read_ = GetCode(expl);
   return read_;
}

//------------------------------------------------------------------------------

fn_name Editor_RenameIncludeGuard = "Editor.RenameIncludeGuard";

word Editor::RenameIncludeGuard(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_RenameIncludeGuard);

   //  This warning is logged against the #define.
   //
   auto def = FindLine(log.line, expl);
   if(def == source_.end()) return 0;
   auto ifn = Rfind(def, HASH_IFNDEF_STR);
   if(ifn.pos == string::npos) return NotFound(expl, HASH_IFNDEF_STR);
   if(def->code.find(HASH_DEFINE_STR) == string::npos)
      return NotFound(expl, HASH_DEFINE_STR);
   auto guard = log.file->MakeGuardName();
   ifn.iter->code.erase(strlen(HASH_IFNDEF_STR) + 1);
   ifn.iter->code.append(guard);
   def->code.erase(strlen(HASH_DEFINE_STR) + 1);
   def->code.append(guard);
   return Changed(def, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_ReplaceNull = "Editor.ReplaceNull";

word Editor::ReplaceNull(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_ReplaceNull);

   //  If there are multiple occurrences on the same line, each one will
   //  cause a log, so just fix the first one.
   //
   auto s = FindLine(log.line, expl);
   if(s == source_.end()) return 0;
   auto null = FindWord(s, 0, NULL_STR);
   if(null.pos == string::npos) return NotFound(expl, NULL_STR, true);
   null.iter->code.erase(null.pos, strlen(NULL_STR));
   null.iter->code.insert(null.pos, NULLPTR_STR);
   return Changed(null.iter, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_ReplaceSlashAsterisk = "Editor.ReplaceSlashAsterisk";

word Editor::ReplaceSlashAsterisk(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_ReplaceSlashAsterisk);

   auto s = FindLine(log.line, expl);
   if(s == source_.end()) return 0;
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
   for(++s; s != source_.end(); ++s)
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

fn_name Editor_ReplaceUsing = "Editor.ReplaceUsing";

word Editor::ReplaceUsing(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_ReplaceUsing);

   //  Before removing the using statement, add type aliases to each class
   //  for symbols that appear in its definition and that were resolved by
   //  a using statement.
   //
   ResolveUsings();
   return EraseUsing(log, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_ResolveUsings = "Editor.ResolveUsings";

word Editor::ResolveUsings()
{
   Debug::ft(Editor_ResolveUsings);

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

Editor::CodeLocation Editor::Rfind(Iter iter, const string& str, size_t off)
{
   if(iter == source_.end()) return CodeLocation(iter);

   auto pos = iter->code.rfind(str, off);

   while(pos == string::npos)
   {
      if(iter == source_.begin()) return CodeLocation(source_.end());
      --iter;
      pos = iter->code.rfind(str);
   }

   return CodeLocation(iter, pos);
}

//------------------------------------------------------------------------------

fn_name Editor_RfindNonBlank = "Editor.RfindNonBlank";

Editor::CodeLocation Editor::RfindNonBlank(Iter iter, size_t pos)
{
   Debug::ft(Editor_RfindNonBlank);

   if(iter == source_.end()) return CodeLocation(iter);
   if(pos == string::npos) pos = iter->code.size();

   while(true)
   {
      for(NO_OP; pos != string::npos; --pos)
      {
         if(!IsBlank(iter->code[pos])) return CodeLocation(iter, pos);
      }

      if(iter == source_.begin()) break;
      --iter;
      pos = iter->code.size();
   }

   return CodeLocation(source_.end());
}

//------------------------------------------------------------------------------

fn_name Editor_SortIncludes = "Editor.SortIncludes";

word Editor::SortIncludes(string& expl)
{
   Debug::ft(Editor_SortIncludes);

   //  Just sort the #include directives.
   //
   auto begin = IncludesBegin();
   if(begin == source_.end()) return NotFound(expl, HASH_INCLUDE_STR);
   auto end = IncludesEnd();

   //  std::list does not support a sort bounded by iterators, so move all of
   //  the #include statements into a new list, sort them, and reinsert them.
   //
   std::list< SourceLine > includes;

   for(auto s = begin; s != end; NO_OP)
   {
      includes.push_back(SourceLine(s->code, s->line));
      s = source_.erase(s);
   }

   includes.sort(IsSorted1);

   for(auto s = includes.begin(); s != includes.end(); ++s)
   {
      source_.insert(end, *s);
   }

   sorted_ = true;
   Changed();
   return Report(expl, "All #includes sorted.");
}

//------------------------------------------------------------------------------

fn_name Editor_TagAsConstArgument = "Editor.TagAsConstArgument";

word Editor::TagAsConstArgument(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_TagAsConstArgument);

   //  Find the line on which the argument's type appears, and insert
   //  "const" before that type.
   //
   auto type = FindPos(log.item->GetTypeSpec()->GetPos());
   if(type.pos == string::npos) return NotFound(expl, "Argument type");
   type.iter->code.insert(type.pos, "const ");
   return Changed(type.iter, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_TagAsConstData = "Editor.TagAsConstData";

word Editor::TagAsConstData(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_TagAsConstData);

   //  Find the line on which the data's type appears, and insert
   //  "const" before that type.
   //
   auto type = FindPos(log.item->GetTypeSpec()->GetPos());
   if(type.pos == string::npos) return NotFound(expl, "Data type");
   type.iter->code.insert(type.pos, "const ");
   return Changed(type.iter, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_TagAsConstFunction = "Editor.TagAsConstFunction";

word Editor::TagAsConstFunction(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_TagAsConstFunction);

   //  Look for the end of the function's signature, which is either
   //  a semicolon or the left brace of an inline function.
   //
   auto func = FindPos(log.item->GetPos());
   if(func.pos == string::npos) return NotFound(expl, "Function name");
   auto endsig = FindFirstOf(func.iter, func.pos, ";{");
   if(endsig.pos == string::npos) return NotFound(expl, "Signature end");

   if(endsig.iter->code[endsig.pos] == '{')
   {
      if(endsig.iter->code.find_first_not_of(WhitespaceChars) == endsig.pos)
      {
         //  The brace is on a new line.  Append " const" to the previous line.
         //
         --endsig.iter;
         endsig.iter->code.append(" const");
      }
      else
      {
         //  The brace is not on a new line, so insert "const " before it.
         //
         endsig.iter->code.insert(endsig.pos, "const ");
      }
   }
   else
   {
      //  If there isn't a space before the insertion point, insert
      //  " const" instead of "const".
      //
      if((endsig.pos > 0) && !IsBlank(endsig.iter->code[endsig.pos - 1]))
         endsig.iter->code.insert(endsig.pos, " const");
      else
         endsig.iter->code.insert(endsig.pos, "const");
   }

   return Changed(endsig.iter, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_TagAsConstPointer = "Editor.TagAsConstPointer";

word Editor::TagAsConstPointer(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_TagAsConstPointer);

   //  If there is more than one pointer, this applies to the last one, so
   //  back up from the data item's name.
   //
   auto data = FindPos(log.item->GetPos());
   if(data.pos == string::npos) return NotFound(expl, "Member name");
   auto ptr = Rfind(data.iter, "*", data.pos);
   if(ptr.pos == string::npos) return NotFound(expl, "Pointer tag");
   ptr.iter->code.insert(ptr.pos + 1, " const");
   return Changed(ptr.iter, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_TagAsConstReference = "Editor.TagAsConstReference";

word Editor::TagAsConstReference(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_TagAsConstReference);

   //  Find the line on which the argument's name appears.  Insert a reference
   //  tag before the argument's name and "const" before its type.  The tag is
   //  added first so that its position won't change as a result of adding the
   //  "const" earlier in the line.
   //
   auto arg = FindPos(log.item->GetPos());
   if(arg.pos == string::npos) return NotFound(expl, "Argument name");
   auto prev = RfindNonBlank(arg.iter, arg.pos - 1);
   prev.iter->code.insert(prev.pos + 1, 1, '&');
   auto rc = TagAsConstArgument(log, expl);
   if(rc != 0) return rc;
   return Changed(prev.iter, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_TagAsDefaulted = "Editor.TagAsDefaulted";

word Editor::TagAsDefaulted(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_TagAsDefaulted);

   //  If this instance of the function (declaration or definition) has
   //  the empty implementation, replace the empty braces.
   //
   if(!static_cast< const Function* >(log.item)->IsImplemented())
   {
      return Report
         (expl, "This change must be made where the function is defined.");
   }

   auto func = FindPos(log.pos);
   auto left = FindFirstOf(func.iter, func.pos, "{");
   auto right = FindFirstOf(func.iter, func.pos, "}");
   left.iter->code.erase(left.pos);
   if(right.iter != left.iter) right.iter->code.erase(0, right.pos + 1);
   left.iter->code.insert(left.pos, "= default;");
   return Changed(left.iter, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_TagAsExplicit = "Editor.TagAsExplicit";

word Editor::TagAsExplicit(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_TagAsExplicit);

   //  A constructor can be tagged "constexpr", which the parser looks for
   //  only *after* "explicit".
   //
   auto ctor = FindPos(log.item->GetPos());
   if(ctor.pos == string::npos) return NotFound(expl, "Constructor");
   auto prev = ctor.iter->code.rfind(CONSTEXPR_STR, ctor.pos);
   if(prev != string::npos) ctor.pos = prev;
   ctor.iter->code.insert(ctor.pos, "explicit ");
   return Changed(ctor.iter, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_TagAsOverride = "Editor.TagAsOverride";

word Editor::TagAsOverride(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_TagAsOverride);

   //  Insert "override" just before the semicolon at the end of the
   //  function's declaration or the left brace at the beginning of
   //  its inline definition.
   //
   auto func = FindPos(log.item->GetPos());
   if(func.pos == string::npos) return NotFound(expl, "Function name");
   auto endsig = FindFirstOf(func.iter, func.pos, ";{");
   if(endsig.pos == string::npos) return NotFound(expl, "Signature end");

   //  Insert "override" after the previous non-blank character.
   //
   endsig = RfindNonBlank(endsig.iter, endsig.pos - 1);
   endsig.iter->code.insert(endsig.pos + 1, " override");
   return Changed(endsig.iter, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_TagAsStaticFunction = "Editor.TagAsStaticFunction";

word Editor::TagAsStaticFunction(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_TagAsStaticFunction);

   //  It's best to start with the function's return type, just in case it's
   //  on the line above the function's name.  Then find the end of the
   //  function's signature and the right parenthesis that precedes it.
   //
   auto type = FindPos(log.item->GetTypeSpec()->GetPos());
   if(type.pos == string::npos) return NotFound(expl, "Function type");
   auto endsig = FindFirstOf(type.iter, type.pos, ";{");
   if(endsig.pos == string::npos) return NotFound(expl, "Signature end");
   auto rpar = Rfind(endsig.iter, ")", endsig.pos);
   if(rpar.pos == string::npos)
      return NotFound(expl, "Closing right parenthesis");

   //  Only a function's declaration, not its definition, is tagged "static".
   //  The parser also wants "static" to follow "extern" and "inline".
   //
   auto func = static_cast< const Function* >(log.item);

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

   //  A static function cannot be const, so remove that tag if it exists.
   //
   if(func->IsConst())
   {
      auto tag = FindWord(rpar.iter, rpar.pos, CONST_STR);
      if(tag.pos != string::npos)
      {
         auto& code = tag.iter->code;
         code.erase(tag.pos, strlen(CONST_STR));
         if((tag.pos < code.size()) && IsBlank(code[tag.pos]))
            code.erase(tag.pos, 1);
      }
   }

   return Changed(type.iter, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_TagAsVirtual = "Editor.TagAsVirtual";

word Editor::TagAsVirtual(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_TagAsVirtual);

   //  A virtual function can be tagged "constexpr", which the parser looks for
   //  only *after* "virtual".
   //
   auto type = FindPos(log.item->GetTypeSpec()->GetPos());
   if(type.pos == string::npos) return NotFound(expl, "Function type");
   auto prev = type.iter->code.rfind(CONSTEXPR_STR, type.pos);
   if(prev != string::npos) type.pos = prev;
   type.iter->code.insert(type.pos, "virtual ");
   return Changed(type.iter, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_UntagAsMutable = "Editor.UntagAsMutable";

word Editor::UntagAsMutable(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_UntagAsMutable);

   //  Find the line on which the data's type appears, and erase the
   //  "mutable" before that type.
   //
   auto type = FindPos(log.item->GetTypeSpec()->GetPos());
   if(type.pos == string::npos) return NotFound(expl, "Data type");
   auto tag = Rfind(type.iter, MUTABLE_STR, type.pos);
   if(tag.pos == string::npos) return NotFound(expl, MUTABLE_STR, true);
   tag.iter->code.erase(tag.pos, strlen(MUTABLE_STR) + 1);
   return Changed(tag.iter, expl);
}

//------------------------------------------------------------------------------

fn_name Editor_Write = "Editor.Write";

word Editor::Write(const string& path, string& expl)
{
   Debug::ft(Editor_Write);

   //  Perform an automatic >format on the file.  In particular, some edits
   //  could have introduced blank line pairs.
   //
   EraseTrailingBlanks();
   EraseBlankLinePairs();
   ConvertTabsToBlanks();

   //  Return if nothing has changeed.
   //
   if(!changed_) return 0;

   //  Create a new file to hold the reformatted version.
   //
   auto temp = path + ".tmp";
   auto output = SysFile::CreateOstream(temp.c_str(), true);
   if(output == nullptr) return Report(expl, "Failed to open output file.", -1);

   //  #includes for files that define base classes used in this file or declare
   //  functions defined in this file had their angle brackets or quotes mangled
   //  for sorting purposes.  Fix this.
   //
   for(auto s = IncludesBegin(); s != IncludesEnd(); ++s)
   {
      s->code = DemangleInclude(s->code);
   }

   for(auto s = source_.cbegin(); s != source_.cend(); ++s)
   {
      *output << s->code << CRLF;
   }

   //  Close both files, delete the original, and replace it with the new one.
   //
   input_.reset();
   output.reset();
   remove(path.c_str());
   auto err = rename(temp.c_str(), path.c_str());
   if(err != 0) return Report(expl, "Failed to rename new file to old.", -1);
   changed_ = false;
   return 1;
}
}
