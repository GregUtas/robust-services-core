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
#include <cstdio>
#include <cstring>
#include <istream>
#include <iterator>
#include <ostream>
#include <utility>
#include "CodeFile.h"
#include "CodeTypes.h"
#include "CodeWarning.h"
#include "Debug.h"
#include "Formatters.h"
#include "Lexer.h"

using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
fn_name Editor_ctor = "Editor.ctor";

Editor::Editor(CodeFile* file, istreamPtr& input) :
   file_(file),
   input_(std::move(input)),
   line_(0),
   changed_(false)
{
   Debug::ft(Editor_ctor);

   input_->clear();
   input_->seekg(0);
}

//------------------------------------------------------------------------------

fn_name Editor_AddForward = "Editor.AddForward";

word Editor::AddForward(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_AddForward);

   //  LOG must provide the namespace for the forward declaration.
   //
   auto qname = log.info;
   auto pos = qname.find(SCOPE_STR);
   if(pos == string::npos) return Report(expl, "Symbol's namespace required.");

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

   string forward = spaces(Indent_Size);
   forward += areaStr;
   forward.push_back(SPACE);
   forward += qname.substr(pos + 2);
   forward.push_back(';');

   //  Set NSPACE to "namespace <ns>", where <ns> is the symbol's namespace.
   //  Then decide where to insert the forward declaration.
   //
   string nspace = NAMESPACE_STR;
   nspace += qname.substr(areaStr.size(), pos - areaStr.size());

   for(auto i = epilog_.begin(); i != epilog_.end(); ++i)
   {
      if(i->code.find(NAMESPACE_STR) == 0)
      {
         //  If this namespace matches NSPACE, add the declaration to it.
         //  If this namespace's name is alphabetically after NSPACE, add
         //  the declaration before it, along with its namespace.
         //
         auto comp = strCompare(i->code, nspace);
         if(comp == 0) return InsertForward(i, forward, expl);
         if(comp > 0) return InsertForward(i, nspace, forward, expl);
      }
      else if((i->code.find(USING_STR) == 0) ||
         (i->code.find(SingleRule) == 0) ||
         (i->code.find(DoubleRule) == 0))
      {
         //  We have now passed any existing forward declarations, so add
         //  the new declaration here, along with its namespace.
         //
         i = epilog_.insert(i, SourceLine(EMPTY_STR, 0));
         return InsertForward(i, nspace, forward, expl);
      }
   }

   return Report(expl, "Failed to insert forward declaration.");
}

//------------------------------------------------------------------------------

fn_name Editor_AddInclude = "Editor.AddInclude";

word Editor::AddInclude(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_AddInclude);

   //  Create the new #include directive and add it to the appropriate list.
   //
   string include = HASH_INCLUDE_STR;
   include.push_back(SPACE);
   include += log.info;
   auto& incls = (include.back() == '>' ? extIncls_ : intIncls_);
   return InsertInclude(incls, include);
}

//------------------------------------------------------------------------------

fn_name Editor_AddUsing = "Editor.AddUsing";

word Editor::AddUsing(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_AddUsing);

   //  Create the new using statement and decide where to insert it.
   //
   string statement = USING_STR;
   statement.push_back(SPACE);
   statement += log.info;
   statement.push_back(';');

   auto usings = false;

   for(auto i = epilog_.begin(); i != epilog_.end(); ++i)
   {
      if(i->code.find(USING_STR) == 0)
      {
         //  If this using statement is alphabetically after STATEMENT,
         //  add the new statement before it.
         //
         usings = true;

         if(strCompare(i->code, statement) > 0)
         {
            Insert(epilog_, i, statement);
            return 0;
         }
      }
      else if((usings && i->code.empty()) ||
         (i->code.find(SingleRule) == 0) ||
         (i->code.find(DoubleRule) == 0))
      {
         //  We have now passed any existing usings statements, so add
         //  the new statement here.
         //
         i = epilog_.insert(i, SourceLine(EMPTY_STR, 0));
         i = Insert(epilog_, i, statement);
         return 0;
      }
   }

   return Report(expl, "Failed to insert using statement.");
}

//------------------------------------------------------------------------------

fn_name Editor_Erase1 = "Editor.Erase[line]";

Editor::Iter Editor::Erase(SourceList& list, size_t line, string& expl)
{
   Debug::ft(Editor_Erase1);

   for(auto i = list.begin(); i != list.end(); ++i)
   {
      if(i->line == line)
      {
         i->code.erase();
         changed_ = true;
         return i;
      }
   }

   expl = "Line not found: " + std::to_string(line);
   return list.end();
}

//------------------------------------------------------------------------------

fn_name Editor_Erase2 = "Editor.Erase[source]";

Editor::Iter Editor::Erase(SourceList& list, const string& source, string& expl)
{
   Debug::ft(Editor_Erase2);

   for(auto i = list.begin(); i != list.end(); ++i)
   {
      if(i->code == source)
      {
         i->code.erase();
         changed_ = true;
         return i;
      }
   }

   //  If we get here, there is another possibility...
   //
   if(prolog_.back().code == source)
   {
      prolog_.pop_back();
      changed_ = true;
      return list.end();
   }

   expl = "Statement not found: " + source;
   return list.end();
}

//------------------------------------------------------------------------------

fn_name Editor_EraseBlankLinePairs = "Editor.EraseBlankLinePairs";

word Editor::EraseBlankLinePairs()
{
   Debug::ft(Editor_EraseBlankLinePairs);

   auto i1 = epilog_.begin();
   if(i1 == epilog_.end()) return 0;

   for(auto i2 = std::next(i1); i2 != epilog_.end(); i2 = std::next(i1))
   {
      if(i1->code.empty() && i2->code.empty())
      {
         i1 = epilog_.erase(i1);
         changed_ = true;
      }
      else
      {
         i1 = i2;
      }
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_EraseEmptyNamespace = "Editor.EraseEmptyNamespace";

word Editor::EraseEmptyNamespace(const Iter& iter)
{
   Debug::ft(Editor_EraseEmptyNamespace);

   //  ITER references the line that follows a forward declaration which was
   //  just deleted.  If this left an empty "namespace <ns> { }", remove it.
   //
   if(iter == epilog_.end()) return 0;
   if(iter->code.find('}') != 0) return 0;

   auto up1 = std::prev(iter);
   if(up1 == epilog_.begin()) return 0;
   auto up2 = std::prev(up1);

   if((up2->code.find(NAMESPACE_STR) == 0) && (up1->code.find('{') == 0))
   {
      epilog_.erase(up2, std::next(iter));
      changed_ = true;
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_EraseTrailingBlanks1 = "Editor.EraseTrailingBlanks";

word Editor::EraseTrailingBlanks()
{
   Debug::ft(Editor_EraseTrailingBlanks1);

   EraseTrailingBlanks(prolog_);
   EraseTrailingBlanks(extIncls_);
   EraseTrailingBlanks(intIncls_);
   EraseTrailingBlanks(epilog_);
   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_EraseTrailingBlanks2 = "Editor.EraseTrailingBlanks[list]";

void Editor::EraseTrailingBlanks(SourceList& list)
{
   Debug::ft(Editor_EraseTrailingBlanks2);

   for(auto i = list.begin(); i != list.end(); ++i)
   {
      while(!i->code.empty() && (i->code.back() == SPACE)) i->code.pop_back();
   }
}

//------------------------------------------------------------------------------

fn_name Editor_Find = "Editor.Find";

Editor::Iter Editor::Find
   (SourceList& list, const Iter& iter, const string& source) const
{
   Debug::ft(Editor_Find);

   for(auto i = iter; i != list.end(); ++i)
   {
      if(i->code == source) return i;
   }

   return list.end();
}

//------------------------------------------------------------------------------

fn_name Editor_GetEpilog = "Editor.GetEpilog";

word Editor::GetEpilog()
{
   Debug::ft(Editor_GetEpilog);

   string str;

   //  Read the remaining lines in the file.
   //
   while(input_->peek() != EOF)
   {
      std::getline(*input_, str);
      PushBack(epilog_, str);
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_GetIncludes = "Editor.GetIncludes";

word Editor::GetIncludes(string& expl)
{
   Debug::ft(Editor_GetIncludes);

   string str;

   //  Resuming after the first #include, add #include directives to one of
   //  two sets, one for external files (e.g. #include <f.h>), and the other
   //  for internal files (e.g. #include "f.h").
   //
   while(input_->peek() != EOF)
   {
      std::getline(*input_, str);

      if(str.find(HASH_INCLUDE_STR) != 0)
      {
         PushBack(epilog_, str);
         return 0;
      }

      auto next = str.find_first_not_of(SPACE, strlen(HASH_INCLUDE_STR));
      if(next == string::npos) return Report(expl, "Empty #include directive.");

      auto& incls = (str[next] == '<' ? extIncls_ : intIncls_);
      PushBack(incls, str);
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_GetProlog = "Editor.GetProlog";

word Editor::GetProlog(string& expl)
{
   Debug::ft(Editor_GetProlog);

   string str;

   //  Read lines up to, and including, the first #include directive.  The
   //  first #include is not moved, under the assumption that it is either
   //  o for a .h*, the base class for the case defined in this file, or
   //  o for a .c*, the .h* that the .c* implements.
   //
   while(input_->peek() != EOF)
   {
      std::getline(*input_, str);
      PushBack(prolog_, str);
      if(str.find(HASH_INCLUDE_STR) == 0) return 0;
   }

   return Report(expl, "File contains no #include statements");
}

//------------------------------------------------------------------------------

fn_name Editor_Insert = "Editor.Insert";

Editor::Iter Editor::Insert(SourceList& list, Iter& iter, const string& source)
{
   Debug::ft(Editor_Insert);

   changed_ = true;
   return list.insert(iter, SourceLine(source, 0));
}

//------------------------------------------------------------------------------

fn_name Editor_InsertForward1 = "Editor.InsertForward";

word Editor::InsertForward
   (const Iter& iter, const string& forward, string& expl)
{
   Debug::ft(Editor_InsertForward1);

   //  ITER references a namespace that matches the one for a new forward
   //  declaration.  Insert the new declaration alphabetically within the
   //  declarations that already appear in this namespace.
   //
   for(auto i = std::next(iter, 2); i != epilog_.end(); ++i)
   {
      if((strCompare(i->code, forward) > 0) ||
         (i->code.find('}') != string::npos))
      {
         Insert(epilog_, i, forward);
         return 0;
      }
   }

   return Report(expl, "Failed to insert forward declaration");
}

//------------------------------------------------------------------------------

fn_name Editor_InsertForward2 = "Editor.InsertForward[ns]";

word Editor::InsertForward
   (Iter& iter, const string& nspace, const string& forward, string& expl)
{
   Debug::ft(Editor_InsertForward2);

   //  Insert a new forward declaration, along with an enclosing namespace,
   //  at ITER.
   //
   auto i = Insert(epilog_, iter, "}");
   i = Insert(epilog_, i, forward);
   i = Insert(epilog_, i, "{");
   i = Insert(epilog_, i, nspace);
   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_InsertInclude = "Editor.InsertInclude";

word Editor::InsertInclude(SourceList& list, const string& include)
{
   Debug::ft(Editor_InsertInclude);

   //  If LIST contains no #include directives, or if its last #include is
   //  alphabetically before the one to be added, insert the new #include
   //  at the end of LIST, else traverse LIST and insert it alphabetically.
   //
   if(list.empty() || strCompare(list.back().code, include) < 0)
   {
      Insert(list, list.end(), include);
   }
   else
   {
      for(auto i = list.begin(); i != list.end(); ++i)
      {
         if(strCompare(i->code, include) > 0)
         {
            Insert(list, i, include);
            break;
         }
      }
   }

   return 0;
}

//------------------------------------------------------------------------------

bool Editor::IsSorted(const SourceLine& line1, const SourceLine& line2)
{
   return (strCompare(line1.code, line2.code) <= 0);
}

//------------------------------------------------------------------------------

void Editor::PushBack(SourceList& list, const string& source)
{
   ++line_;
   list.push_back(SourceLine(source, line_));
}

//------------------------------------------------------------------------------

fn_name Editor_Read = "Editor.Read";

word Editor::Read(string& expl)
{
   Debug::ft(Editor_Read);

   auto rc = GetProlog(expl);
   if(rc != 0) return rc;
   rc = GetIncludes(expl);
   if(rc != 0) return rc;
   return GetEpilog();
}

//------------------------------------------------------------------------------

fn_name Editor_RemoveForward = "Editor.RemoveForward";

word Editor::RemoveForward(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_RemoveForward);

   //  Erase the line where the forward declaration appears.
   //  If this succeeds, delete the line entirely, and then
   //  delete the enclosing namespace if it is now empty.
   //
   auto iter = Erase(epilog_, log.line + 1, expl);
   if(iter != epilog_.end()) iter = epilog_.erase(iter);
   return EraseEmptyNamespace(iter);
}

//------------------------------------------------------------------------------

fn_name Editor_RemoveInclude = "Editor.RemoveInclude";

word Editor::RemoveInclude(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_RemoveInclude);

   //  Extract the code where the #include directive appears in
   //  order to determine which list to remove it from.  If it
   //  appeared in that list, delete its line entirely.
   //
   auto source = file_->GetLexer().GetNthLine(log.line);
   if(source.empty()) return Report(expl, "#include not found");
   auto& incls = (source.back() == '>' ? extIncls_ : intIncls_);
   auto iter = Erase(incls, source, expl);
   if(iter != incls.end()) incls.erase(iter);
   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_RemoveUsing = "Editor.RemoveUsing";

word Editor::RemoveUsing(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_RemoveUsing);

   //  Erase the line where the using statement appears.  If the
   //  statement appeared there, delete its line entirely.
   //
   auto iter = Erase(epilog_, log.line + 1, expl);
   if(iter != epilog_.end()) epilog_.erase(iter);
   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_Report = "Editor.Report";

word Editor::Report(string& expl, fixed_string text, word rc)
{
   Debug::ft(Editor_Report);

   expl = text;
   return rc;
}

//------------------------------------------------------------------------------

fn_name Editor_SortIncludes = "Editor.SortIncludes";

word Editor::SortIncludes(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_SortIncludes);

   //  LOG specifies an unsorted #include directive.  Just sort all of the
   //  #includes in its group.
   //
   auto source = file_->GetLexer().GetNthLine(log.line);
   if(source.find(HASH_INCLUDE_STR) != 0)
      return Report(expl, "No \"#include\" in unsorted #include directive");
   auto& incls = (source.find('<') != string::npos ? extIncls_ : intIncls_);
   incls.sort(IsSorted);
   changed_ = true;
   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_Write = "Editor.Write";

word Editor::Write(const string& path, string& expl)
{
   Debug::ft(Editor_Write);

   //  Return if nothing has changeed.
   //
   if(!changed_) return 0;

   //  Create a new file to hold the reformatted version.
   //
   auto file = path + ".tmp";
   auto output = ostreamPtr(SysFile::CreateOstream(file.c_str(), true));
   if(output == nullptr) return Report(expl, "Failed to open output file.", -7);

   EraseBlankLinePairs();
   EraseTrailingBlanks();

   for(auto s = prolog_.cbegin(); s != prolog_.cend(); ++s)
   {
      *output << s->code << CRLF;
   }

   for(auto s = extIncls_.cbegin(); s != extIncls_.cend(); ++s)
   {
      *output << s->code << CRLF;
   }

   for(auto s = intIncls_.cbegin(); s != intIncls_.cend(); ++s)
   {
      *output << s->code << CRLF;
   }

   for(auto s = epilog_.cbegin(); s != epilog_.cend(); ++s)
   {
      *output << s->code << CRLF;
   }

   //  Close both files, delete the original, and replace it with the new one.
   //
   input_.reset();
   output.reset();
   remove(path.c_str());
   auto err = rename(file.c_str(), path.c_str());
   if(err != 0) return Report(expl, "Failed to rename new file to old.", -6);
   file_->SetModified();
   return 1;
}
}
