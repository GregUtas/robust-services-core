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
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <istream>
#include <iterator>
#include <ostream>
#include <utility>
#include <vector>
#include "CodeFile.h"
#include "CodeTypes.h"
#include "CodeWarning.h"
#include "Cxx.h"
#include "CxxArea.h"
#include "CxxScope.h"
#include "CxxScoped.h"
#include "CxxToken.h"
#include "Debug.h"
#include "Formatters.h"
#include "Lexer.h"

using namespace NodeBase;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
const string Editor::FrontChars = "$%@!<\"";
const string Editor::BackChars = "$%@!>\"";

//------------------------------------------------------------------------------

fn_name Editor_ctor = "Editor.ctor";

Editor::Editor(CodeFile* file, istreamPtr& input) :
   file_(file),
   input_(std::move(input)),
   line_(0),
   changed_(false),
   aliased_(false)
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
         if(comp > 0) return InsertNamespaceForward(i, nspace, forward);
      }
      else if((i->code.find(USING_STR) == 0) ||
         (i->code.find(SingleRule) == 0) ||
         (i->code.find(DoubleRule) == 0))
      {
         //  We have now passed any existing forward declarations, so add
         //  the new declaration here, along with its namespace.
         //
         Insert(epilog_, i, EMPTY_STR);
         return InsertNamespaceForward(i, nspace, forward);
      }
   }

   return Report(expl, "Failed to insert forward declaration.");
}

//------------------------------------------------------------------------------

fn_name Editor_AddInclude = "Editor.AddInclude";

word Editor::AddInclude(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_AddInclude);

   //  Create the new #include directive and add it to the list.
   //  Its file name appears in log.info.
   //
   string include = HASH_INCLUDE_STR;
   include.push_back(SPACE);
   include += log.info;
   return InsertInclude(include, expl);
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
         i = Insert(epilog_, i, EMPTY_STR);
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

   auto iter = Find(list, line);

   if(iter != list.end())
   {
      iter = list.erase(iter);
      changed_ = true;
      return iter;
   }

   expl = "Line not found: " + std::to_string(line + 1);
   return iter;
}

//------------------------------------------------------------------------------

fn_name Editor_Erase2 = "Editor.Erase[source]";

Editor::Iter Editor::Erase(SourceList& list, const string& source, string& expl)
{
   Debug::ft(Editor_Erase2);

   auto iter = Find(list, source);

   if(iter != list.end())
   {
      iter = list.erase(iter);
      changed_ = true;
      return iter;
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
   EraseTrailingBlanks(includes_);
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
      while(!i->code.empty() && (i->code.back() == SPACE))
      {
         i->code.pop_back();
         changed_ = true;
      }
   }
}

//------------------------------------------------------------------------------

Editor::Iter Editor::Find(SourceList& list, size_t line)
{
   for(auto i = list.begin(); i != list.end(); ++i)
   {
      if(i->line == line) return i;
   }

   return list.end();
}

//------------------------------------------------------------------------------

Editor::Iter Editor::Find(SourceList& list, const string& source)
{
   for(auto i = list.begin(); i != list.end(); ++i)
   {
      if(i->code == source) return i;
   }

   return list.end();
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

   //  Resuming after the first #include, add #include directives until
   //  something else is reached.
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
      if(next == string::npos)
         return Report(expl, "Empty #include directive in file.", -1);
      PushInclude(str, expl);
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_GetProlog = "Editor.GetProlog";

word Editor::GetProlog(string& expl)
{
   Debug::ft(Editor_GetProlog);

   string str;

   //  Read lines up to the first #include directive.
   //
   while(input_->peek() != EOF)
   {
      std::getline(*input_, str);

      if(str.find(HASH_INCLUDE_STR) == 0)
      {
         PushInclude(str, expl);
         return 0;
      }

      PushBack(prolog_, str);
   }

   return Report(expl, "No #include directives in file.");
}

//------------------------------------------------------------------------------

fn_name Editor_Insert = "Editor.Insert";

Editor::Iter Editor::Insert(SourceList& list, Iter& iter, const string& source)
{
   Debug::ft(Editor_Insert);

   changed_ = true;
   return list.insert(iter, SourceLine(source, SIZE_MAX));
}

//------------------------------------------------------------------------------

fn_name Editor_InsertForward = "Editor.InsertForward";

word Editor::InsertForward
   (const Iter& iter, const string& forward, string& expl)
{
   Debug::ft(Editor_InsertForward);

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

   return Report(expl, "Failed to insert forward declaration.");
}

//------------------------------------------------------------------------------

fn_name Editor_InsertInclude = "Editor.InsertInclude";

word Editor::InsertInclude(string& include, string& expl)
{
   Debug::ft(Editor_InsertInclude);

   //  Modify the characters that surround the #include's file name
   //  based on the group to which it belongs.
   //
   if(MangleInclude(include, expl) != 0) return 0;

   //  If there are no #include directives, or if the last one should
   //  precede the new one, insert the new #include at the end of the
   //  list, else traverse the list and insert it in its proper place.
   //
   if(includes_.empty() || IsSorted2(includes_.back().code, include))
   {
      Insert(includes_, includes_.end(), include);
   }
   else
   {
      for(auto i = includes_.begin(); i != includes_.end(); ++i)
      {
         if(!IsSorted2(i->code, include))
         {
            Insert(includes_, i, include);
            break;
         }
      }
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_InsertNamespaceForward = "Editor.InsertNamespaceForward";

word Editor::InsertNamespaceForward
   (Iter& iter, const string& nspace, const string& forward)
{
   Debug::ft(Editor_InsertNamespaceForward);

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
   if(pos2 == string::npos) return true;
   if(pos1 == string::npos) return false;
   auto c1 = line1[pos1];
   auto c2 = line2[pos2];
   auto group1 = FrontChars.find(c1);
   auto group2 = FrontChars.find(c2);
   if(group1 < group2) return true;
   if(group1 == group2) return (strCompare(line1, line2) <= 0);
   return false;
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

void Editor::PushBack(SourceList& list, const string& source)
{
   list.push_back(SourceLine(source, line_));
   ++line_;
}

//------------------------------------------------------------------------------

word Editor::PushInclude(string& source, string& expl)
{
   if(MangleInclude(source, expl) != 0) return 0;
   PushBack(includes_, source);
   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_QualifyReferent = "Editor.QualifyReferent";

void Editor::QualifyReferent(const CxxNamed* item, const CxxNamed* ref)
{
   Debug::ft(Editor_QualifyReferent);

   //  Within ITEM, prefix NS wherever SYMBOL appears as an identifier.
   //
   auto ns = ref->GetSpace();
   auto symbol = ref->Name();

   switch(ref->Type())
   {
   case Cxx::Namespace:
      ns = static_cast< Namespace* >
         (const_cast< CxxNamed* >(ref))->OuterSpace();
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
   auto lexer = file_->GetLexer();
   item->GetRange(pos, end);
   lexer.Reposition(pos);
   string name;

   while((lexer.Curr() < end) && lexer.FindIdentifier(name))
   {
      if(name != *symbol)
      {
         lexer.Advance(name.size());
         continue;
      }

      //  This line contains at least one occurrence of SYMBOL.  Qualify
      //  each occurrence with NS if it is neither qualified nor part of
      //  a longer identifer.
      //
      auto line = lexer.GetLineNum(lexer.Curr());
      auto targ = Find(epilog_, line);
      auto& code = targ->code;

      for(pos = code.find(*symbol); pos != string::npos;
         pos = code.find(*symbol, pos))
      {
         if((code.rfind(SCOPE_STR, pos) != pos - 2) &&
            (ValidNextChars.find(code[pos - 1]) == string::npos) &&
            (ValidNextChars.find(code[pos + name.size()]) == string::npos))
         {
            auto qual = ns->ScopedName(false);
            qual.append(SCOPE_STR);
            code.insert(pos, qual);
            changed_ = true;
            pos += qual.size();
         }

         pos += name.size();  // always bypass current occurrence of NAME
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

   //  Erase the line where the forward declaration appears.  This
   //  may leave an empty enclosing namespace that should be deleted.
   //
   auto iter = Erase(epilog_, log.line, expl);
   return EraseEmptyNamespace(iter);
}

//------------------------------------------------------------------------------

fn_name Editor_RemoveInclude = "Editor.RemoveInclude";

word Editor::RemoveInclude(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_RemoveInclude);

   //  Extract the code where the #include directive appears in
   //  order to determine which list to remove it from.
   //
   auto source = file_->GetLexer().GetNthLine(log.line);
   if(source.empty()) return Report(expl, "#include not found");
   if(MangleInclude(source, expl) != 0) return 0;
   Erase(includes_, source, expl);
   return 0;
}

//------------------------------------------------------------------------------

fn_name Editor_RemoveUsing = "Editor.RemoveUsing";

word Editor::RemoveUsing(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_RemoveUsing);

   //  Erase the line where the using statement appears.
   //
   Erase(epilog_, log.line, expl);
   return 0;
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
   ResolveUsings(log, expl);
   return RemoveUsing(log, expl);
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

fn_name Editor_ResolveUsings = "Editor.ResolveUsings";

word Editor::ResolveUsings(const WarningLog& log, std::string& expl)
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

fn_name Editor_SortIncludes = "Editor.SortIncludes";

word Editor::SortIncludes(const WarningLog& log, string& expl)
{
   Debug::ft(Editor_SortIncludes);

   //  LOG specifies an unsorted #include directive.  Just sort all of the
   //  #includes.
   //
   auto source = file_->GetLexer().GetNthLine(log.line);
   if(source.find(HASH_INCLUDE_STR) != 0)
      return Report(expl, "No \"#include\" in unsorted #include directive.");
   includes_.sort(IsSorted1);
   changed_ = true;
   return Report(expl, "All #includes sorted.");
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
   if(output == nullptr) return Report(expl, "Failed to open output file.", -1);

   EraseBlankLinePairs();
   EraseTrailingBlanks();

   for(auto s = prolog_.cbegin(); s != prolog_.cend(); ++s)
   {
      *output << s->code << CRLF;
   }

   for(auto s = includes_.begin(); s != includes_.end(); ++s)
   {
      //  #includes belonging to declIds_ or baseIds_ had their angle
      //  brackets or quotes replaced for sorting purposes.  Fix this.
      //
      auto pos = s->code.find_first_of(FrontChars);
      if(pos == string::npos) return Report(expl, "Error in #include.", -1);

      switch(FrontChars.find_first_of(s->code[pos]))
      {
      case 0:
      case 2:
         s->code[pos] = '<';
         s->code.back() = '>';
         break;
      case 1:
      case 3:
         s->code[pos] = QUOTE;
         s->code.back() = QUOTE;
         break;
      }

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
   if(err != 0) return Report(expl, "Failed to rename new file to old.", -1);
   file_->SetModified();
   return 1;
}
}
