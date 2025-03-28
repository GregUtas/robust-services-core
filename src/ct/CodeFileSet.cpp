//==============================================================================
//
//  CodeFileSet.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "CodeFileSet.h"
#include <algorithm>
#include <cstddef>
#include <iosfwd>
#include <iterator>
#include <sstream>
#include "CliThread.h"
#include "CodeDir.h"
#include "CodeDirSet.h"
#include "CodeFile.h"
#include "CodeItemSet.h"
#include "CodeTypes.h"
#include "CodeWarning.h"
#include "Cxx.h"
#include "CxxExecute.h"
#include "CxxFwd.h"
#include "CxxNamed.h"
#include "Debug.h"
#include "Editor.h"
#include "Formatters.h"
#include "Library.h"
#include "NbCliParms.h"
#include "Parser.h"
#include "SetOperations.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "ThisThread.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Returns the set of files that need to be parsed when parsing FILES.
//  It adds files that affect FILES and then removes files that have
//  already been parsed.
//
static LibrarySet* GetParseSet(const LibItemSet& files)
{
   Debug::ft("CodeTools.GetParseSet");

   //  Expand FILES with substitute files and those that affect FILES.
   //
   auto library = Singleton<Library>::Instance();
   LibItemSet parseSet(files);
   SetUnion(parseSet, library->SubsFiles().Items());

   auto parseFiles = new CodeFileSet(LibrarySet::TemporaryName(), &parseSet);
   auto affects = parseFiles->Affecters();
   auto& items = affects->Items();

   //  Remove files that have already been parsed.
   //
   for(auto f = items.begin(); f != items.end(); NO_OP)
   {
      auto file = static_cast<CodeFile*>(*f);

      if(file->ParseStatus() != CodeFile::Unparsed)
         f = items.erase(f);
      else
         ++f;
   }

   return affects;
}

//------------------------------------------------------------------------------

static bool IsSortedByFileLevel(const FileLevel& item1, const FileLevel& item2)
{
   if(item1.level < item2.level) return true;
   if(item1.level > item2.level) return false;
   auto result = strCompare(item1.file->Path(), item2.file->Path());
   if(result == -1) return true;
   if(result == 1) return false;
   return (&item1 < &item2);
}

//==============================================================================

CodeFileSet::CodeFileSet(const string& name, const LibItemSet* items) :
   CodeSet(name, items)
{
   Debug::ft("CodeFileSet.ctor");
}

//------------------------------------------------------------------------------

CodeFileSet::~CodeFileSet()
{
   Debug::ftnt("CodeFileSet.dtor");
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::AffectedBy() const
{
   Debug::ft("CodeFileSet.AffectedBy");

   //  What is affected by this set are those that include it, transitively.
   //  Start with the initial set and add files that directly include any
   //  member of the set.
   //
   LibrarySet* prev = nullptr;
   auto curr = Users(true);
   size_t prevSize = this->Items().size();
   size_t currSize = curr->Items().size();

   //  Keep adding files that #include the new members until the set stops
   //  growing.
   //
   while(prevSize < currSize)
   {
      prev = curr;
      prevSize = currSize;
      curr = prev->Users(true);
      currSize = curr->Items().size();
   }

   return curr;
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::Affecters() const
{
   Debug::ft("CodeFileSet.Affecters");

   //  What affects this set are what it includes, transitively.  Start with
   //  the initial set and add files that any member directly includes.
   //
   LibrarySet* prev = nullptr;
   auto curr = UsedBy(true);
   size_t prevSize = this->Items().size();
   size_t currSize = curr->Items().size();

   //  Keep adding files that the new members #include until the set stops
   //  growing.
   //
   while(prevSize < currSize)
   {
      prev = curr;
      prevSize = currSize;
      curr = prev->UsedBy(true);
      currSize = curr->Items().size();
   }

   return curr;
}

//------------------------------------------------------------------------------

word CodeFileSet::Check(CliThread& cli, ostream* stream, string& expl) const
{
   Debug::ft("CodeFileSet.Check");

   word rc = 0;

   const auto& fileSet = Items();

   if(fileSet.empty())
   {
      expl = EmptySet;
      return rc;
   }

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = static_cast<CodeFile*>(*f);

      if(file->ParseStatus() != CodeFile::Passed)
      {
         expl = "Files to be checked must first be successfully parsed.";
         return rc;
      }
   }

   //  To avoid generating spurious warnings, the following must be parsed:
   //  (a) files affected by those to be checked;
   //  (b) files that affect those in (a).
   //
   auto inclSet = new CodeFileSet(TemporaryName(), &fileSet);
   auto abSet = this->AffectedBy();
   SetUnion(inclSet->Items(), abSet->Items());
   auto parseSet = GetParseSet(inclSet->Items());
   auto& parseItems = parseSet->Items();
   auto skip = true;

   if(parseItems.size() > 0)
   {
      *cli.obuf << parseItems.size() << " files should be parsed to";
      *cli.obuf << CRLF << " avoid spurious results. ";
      skip = cli.BoolPrompt("Do you wish to skip this?");
   }

   if(!skip)
   {
      auto parseFiles = new CodeFileSet(TemporaryName(), &parseSet->Items());
      rc = parseFiles->Parse(expl, "-");
   }

   if(rc != 0) return rc;
   expl.clear();

   CodeWarning::GenerateReport(stream, fileSet);

   std::ostringstream summary;
   summary << fileSet.size() << " file(s) checked.";
   expl = summary.str();
   return rc;
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::CommonAffecters() const
{
   Debug::ft("CodeFileSet.CommonAffecters");

   //  The common affecters of this set is the intersection of
   //  each file's affecters.
   //
   const auto& fileSet = Items();
   auto result = new CodeFileSet(TemporaryName(), nullptr);
   auto& caSet = result->Items();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = static_cast<CodeFile*>(*f);

      if(f == fileSet.cbegin())
         SetUnion(caSet, file->Affecters());
      else
         SetIntersection(caSet, file->Affecters());
      if(caSet.empty()) break;
   }

   return result;
}

//------------------------------------------------------------------------------

word CodeFileSet::Countlines(string& result) const
{
   Debug::ft("CodeFileSet.Countlines");

   result = "linecount: ";

   size_t count = 0;
   const auto& fileSet = Items();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = static_cast<CodeFile*>(*f);
      count += file->GetLexer().LineCount();
   }

   result += std::to_string(count);
   return 0;
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::Create
   (const string& name, const LibItemSet* items) const
{
   Debug::ft("CodeFileSet.Create");

   return new CodeFileSet(name, items);
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::DeclaredBy() const
{
   Debug::ft("CodeFileSet.DeclaredBy");

   const auto& fileSet = Items();
   auto result = new CodeItemSet(TemporaryName(), nullptr);
   auto& declSet = result->Items();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      std::set<CxxNamed*> items;
      auto file = static_cast<CodeFile*>(*f);
      file->GetDecls(items);

      for(auto i = items.cbegin(); i != items.cend(); ++i)
      {
         declSet.insert(*i);
      }
   }

   return result;
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::Directories() const
{
   Debug::ft("CodeFileSet.Directories");

   //  Iterate over the set of code files to find their directories.
   //
   const auto& fileSet = Items();
   auto result = new CodeDirSet(TemporaryName(), nullptr);
   auto& dirSet = result->Items();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = static_cast<CodeFile*>(*f);
      auto dir = file->Dir();
      if(dir != nullptr) dirSet.insert(dir);
   }

   return result;
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::FileName(const LibrarySet* that) const
{
   Debug::ft("CodeFileSet.FileName");

   const auto& fileSet = Items();
   auto result = new CodeFileSet(TemporaryName(), nullptr);

   //  THAT's name encodes the desired filename (e.g. "Sys").
   //
   const auto& fn = that->Name();
   if(fn.empty()) return result;

   //  Iterate over the set of code files to find those that begin with "fn".
   //
   auto& fnSet = result->Items();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = static_cast<CodeFile*>(*f);

      if(file->Name().find(fn) == 0)
      {
         fnSet.insert(file);
      }
   }

   return result;
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::Files() const
{
   Debug::ft("CodeFileSet.Files");

   //  Return the same set.
   //
   return new CodeFileSet(TemporaryName(), &Items());
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::FileType(const LibrarySet* that) const
{
   Debug::ft("CodeFileSet.FileType");

   const auto& fileSet = Items();
   auto result = new CodeFileSet(TemporaryName(), nullptr);

   //  THAT's name encodes the desired filetype (e.g. "cpp").
   //
   auto ft = that->Name();
   if(ft.empty()) return result;
   ft = "." + ft;

   //  Iterate over the set of code files to find those that end with ".ft".
   //
   auto& ftSet = result->Items();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = static_cast<CodeFile*>(*f);
      const auto& name = file->Name();
      auto nsize = name.size();
      auto fsize = ft.size();

      if((nsize > fsize) && (name.rfind(ft) == nsize - fsize))
      {
         ftSet.insert(file);
      }
   }

   return result;
}

//------------------------------------------------------------------------------

word CodeFileSet::Fix(CliThread& cli, FixOptions& opts, string& expl) const
{
   Debug::ft("CodeFileSet.Fix");

   static bool invoked = false;

   const auto& fileSet = Items();

   if(fileSet.empty())
   {
      expl = EmptySet;
      return 0;
   }

   opts.multiple = (fileSet.size() > 1);

   if(!invoked)
   {
      *cli.obuf << "Checking diffs after fixing code is recommended." << CRLF;
      *cli.obuf << "The following is also automatic in modified files:" << CRLF;
      *cli.obuf << "  o Whitespace at the end of a line is deleted." << CRLF;
      *cli.obuf << "  o A repeated blank line is deleted." << CRLF;
      *cli.obuf << "  o If absent, an endline is appended to the file." << CRLF;
      invoked = true;
   }

   //  In order to fix warnings in a file, it must have been checked.
   //
   auto rc = Check(cli, nullptr, expl);
   if(rc != 0) return rc;
   expl.clear();

   //  Iterate over the set of code files and fix them.
   //
   auto prev = Editor::CommitCount();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = static_cast<CodeFile*>(*f);
      rc = file->Fix(cli, opts, expl);
      if(rc != 0) return rc;
   }

   auto changed = Editor::CommitCount() - prev;
   *cli.obuf << changed << " file(s) changed." << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

word CodeFileSet::Format(string& expl) const
{
   Debug::ft("CodeFileSet.Format");

   const auto& fileSet = Items();
   size_t failed = 0;
   string err;

   //  Iterate over the set of code files and reformat them.
   //
   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      err.clear();

      auto file = static_cast<CodeFile*>(*f);

      if(file->GetLexer().LineCount() > 0)
      {
         auto rc = file->Format(err);
         if(rc < 0) ++failed;
         Debug::Progress((rc >= 0 ? CRLF_STR : " ERROR: " + err + CRLF));
      }
   }

   std::ostringstream stream;
   stream << "Total=" << fileSet.size();
   if(failed > 0) stream << ", failed=" << failed;
   expl += stream.str();
   return 0;
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::FoundIn(const LibrarySet* that) const
{
   Debug::ft("CodeFileSet.FoundIn");

   //  Iterate over the set of code files to find those which appear
   //  in one of THAT's directories.
   //
   const auto& dirSet = static_cast<const CodeDirSet*>(that)->Items();
   const auto& fileSet = Items();
   auto result = new CodeFileSet(TemporaryName(), nullptr);
   auto& foundSet = result->Items();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = static_cast<CodeFile*>(*f);
      auto dir = file->Dir();

      if(dir != nullptr)
      {
         if(dirSet.find(dir) != dirSet.cend()) foundSet.insert(file);
      }
   }

   return result;
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::Implements() const
{
   Debug::ft("CodeFileSet.Implements");

   //  In order to find where something declared in a file is defined, and
   //  vice versa, everything that affects the file, and that is affected
   //  by it, must have been parsed.
   //
   auto abSet = this->AffectedBy();
   auto asSet = this->Affecters();
   LibItemSet parseSet;
   SetUnion(parseSet, abSet->Items(), asSet->Items());

   string expl;
   auto parseFiles = new CodeFileSet(TemporaryName(), &parseSet);
   parseFiles->Parse(expl, "-");

   //  Iterate over the set of code files, adding files that implement ones
   //  already in the set.
   //
   const auto& fileSet = Items();
   auto result = new CodeFileSet(TemporaryName(), nullptr);
   auto& imSet = result->Items();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = static_cast<CodeFile*>(*f);
      SetUnion(imSet, file->Implementers());
   }

   return result;
}

//------------------------------------------------------------------------------

word CodeFileSet::LineTypes(CliThread& cli, ostream* stream, string& expl) const
{
   Debug::ft("CodeFileSet.LineTypes");

   word rc = 0;

   const auto& fileSet = Items();

   if(fileSet.empty())
   {
      expl = EmptySet;
      return rc;
   }

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = static_cast<CodeFile*>(*f);

      if(file->ParseStatus() != CodeFile::Passed)
      {
         expl = "Files to be included must first be successfully parsed.";
         return rc;
      }
   }

   CodeFile::DisplayLineTypes(stream, fileSet);

   std::ostringstream summary;
   summary << fileSet.size() << " file(s) counted.";
   expl = summary.str();
   return rc;
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::MatchString(const LibrarySet* that) const
{
   Debug::ft("CodeFileSet.MatchString");

   const auto& fileSet = Items();
   auto result = new CodeFileSet(TemporaryName(), nullptr);

   //  THAT's name is the string to be searched for.
   //
   const auto& s = that->Name();
   if(s.empty()) return result;

   //  Iterate over the set of code files to find those that contains S.
   //
   auto& msSet = result->Items();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = static_cast<CodeFile*>(*f);
      const auto& code = file->GetCode();
      if(code.find(s) != string::npos) msSet.insert(*f);
   }

   return result;
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::NeededBy() const
{
   Debug::ft("CodeFileSet.NeededBy");

   //  The code files needed by the set fs1 are those that must also appear
   //  in a build which contains fs1 in order to resolve all symbols during
   //  linking.  That set is found by repeating >assign fs1 im as fs1 until
   //  the set stops growing.
   //
   size_t prevCount = 0;
   size_t currCount = Items().size();
   LibrarySet* nbSet = (LibrarySet*) this;
   LibrarySet* asSet = nullptr;

   while(prevCount < currCount)
   {
      asSet = nbSet->Affecters();
      nbSet = asSet->Implements();
      if(nbSet == nullptr) return nbSet;
      prevCount = currCount;
      currCount = nbSet->Items().size();
   }

   return nbSet;
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::Needers() const
{
   Debug::ft("CodeFileSet.Needers");

   //  The code files that need any in set fs1 are those that could not appear
   //  in a build without including fs1 to resolve all symbols during linking.
   //  That set of files is found by repeating >assign fs1 im ab fs1 until the
   //  set stops growing.
   //
   size_t prevCount = 0;
   size_t currCount = Items().size();
   LibrarySet* nsSet = (LibrarySet*) this;
   LibrarySet* abSet = nullptr;

   while(prevCount < currCount)
   {
      abSet = nsSet->AffectedBy();
      nsSet = abSet->Implements();
      if(nsSet == nullptr) return nsSet;
      prevCount = currCount;
      currCount = nsSet->Items().size();
   }

   return nsSet;
}

//------------------------------------------------------------------------------

word CodeFileSet::Parse(string& expl, const string& opts) const
{
   Debug::ft("CodeFileSet.Parse");

   const auto& fileSet = Items();

   if(fileSet.empty())
   {
      expl = EmptySet;
      return 0;
   }

   auto parseSet = GetParseSet(fileSet);
   auto order = parseSet->SortInBuildOrder();

   //  Parse substitute files first, followed by the .h's.  This allows the
   //  symbols visible to a file to be determined before parsing it.  Using
   //  declarations in #included files affect visibility, so an #included
   //  file must already have been parsed.
   //
   Context::SetOptions(opts);
   ParserPtr parser(new Parser());
   size_t total = 0;
   size_t failed = 0;

   for(auto f = order.cbegin(); f != order.cend(); ++f)
   {
      if(f->file->IsSubsFile())
      {
         if(!parser->Parse(*f->file)) ++failed;
         ++total;
         ThisThread::Pause();
      }
   }

   for(auto f = order.cbegin(); f != order.cend(); ++f)
   {
      if(f->file->IsSubsFile()) continue;

      if(f->file->IsHeader() && !f->file->IsSubsFile())
      {
         if(!parser->Parse(*f->file)) ++failed;
         ++total;
         ThisThread::Pause();
      }
   }

   for(auto f = order.cbegin(); f != order.cend(); ++f)
   {
      if(f->file->IsCpp() && !f->file->IsSubsFile())
      {
         if(!parser->Parse(*f->file)) ++failed;
         ++total;
         ThisThread::Pause();
      }
   }

   parser.reset();

   //  Update the cross-reference with symbols in the files just parsed.
   //
   if(!order.empty())
   {
      Debug::Progress(string("Updating cross-reference...") + CRLF);

      for(auto f = order.cbegin(); f != order.cend(); ++f)
      {
         if(!f->file->IsSubsFile()) f->file->UpdateXref(true);
      }
   }

   std::ostringstream summary;
   summary << "Total=" << total << ", failed=" << failed;
   expl = summary.str();
   return 0;
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::ReferencedBy() const
{
   Debug::ft("CodeFileSet.ReferencedBy");

   const auto& fileSet = Items();
   auto result = new CodeItemSet(TemporaryName(), nullptr);

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = static_cast<CodeFile*>(*f);
      CxxUsageSets usages;
      file->GetUsageInfo(usages);
      result->CopyUsages(usages);
   }

   return result;
}

//------------------------------------------------------------------------------

word CodeFileSet::Scan
   (ostream& stream, const string& pattern, string& expl) const
{
   Debug::ft("CodeFileSet.Scan");

   const auto& fileSet = Items();

   if(fileSet.empty())
   {
      stream << EmptySet << CRLF;
      return 0;
   }

   stream << "Searching for \"" << pattern << QUOTE << CRLF;

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = static_cast<CodeFile*>(*f);
      const auto& code = file->GetCode();

      auto pos = code.find(pattern);
      auto shown = false;

      while(pos != string::npos)
      {
         if(!shown)
         {
            stream << file->Path(false) << ':' << CRLF;
            shown = true;
         }

         auto line = file->GetLexer().GetLineNum(pos);
         auto str = file->GetLexer().GetNthLine(line);

         stream << spaces(2) << line + 1 << ": " << str << CRLF;
         pos = code.find(pattern, pos + pattern.size());
      }
   }

   return 0;
}

//------------------------------------------------------------------------------

word CodeFileSet::Sort(ostream& stream, string& expl) const
{
   Debug::ft("CodeFileSet.Sort");

   //  Get the build order.
   //
   auto order = SortInBuildOrder();

   if(order.empty())
   {
      expl = EmptySet;
      return 0;
   }

   //  List the files in the original set, showing their build level.
   //
   auto heading = false;
   word room = 65;

   const auto& fileSet = Items();
   size_t shown = 0;
   size_t level = order.front().level + 1;  // to cause mismatch

   for(auto f = order.cbegin(); f != order.cend(); ++f)
   {
      //  List the files that were just included in the build.  Limit
      //  each line to COUT_LENGTH_MAX characters.
      //
      if(f->level != level)
      {
         level = f->level;
         heading = true;
         if(shown > 0) stream << CRLF;
      }

      if(fileSet.find(f->file) == fileSet.cend()) continue;

      if(heading)
      {
         stream << "LEVEL " << level << ':';
         heading = false;
         room = COUT_LENGTH_MAX - 11;
      }
      else
      {
         stream << ',';
      }

      const auto& name = f->file->Name();
      auto size = name.size();

      if(room - size < 2)
      {
         stream << CRLF;
         stream << spaces(2) << name;
         room = COUT_LENGTH_MAX - 2 - (size + 2);
      }
      else
      {
         stream << SPACE << name;
         room -= (size + 2);
      }

      ++shown;
   }

   if(!heading) stream << CRLF;
   stream << "Files shown: " << shown << CRLF;
   return 0;
}

//------------------------------------------------------------------------------

std::vector<CodeFile*> CodeFileSet::SortInAlphaOrder() const
{
   Debug::ft("CodeFileSet.SortInAlphaOrder");

   const auto& fileSet = Items();
   std::vector<CodeFile*> files;

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      files.push_back(static_cast<CodeFile*>(*f));
   }

   std::sort(files.begin(), files.end(), IsSortedByName);
   return files;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_SortInBuildOrder = "CodeFileSet.SortInBuildOrder";

BuildOrder CodeFileSet::SortInBuildOrder() const
{
   Debug::ft(CodeFileSet_SortInBuildOrder);

   //  Create the vector INCLS and clone every file's #include list into it.
   //  The parallel vector FIDS contains the file associated with each entry
   //  in INCLS.
   //
   const auto& fileSet = Items();
   const auto& fullSet = Singleton<Library>::Instance()->Files().Items();
   auto size = fullSet.size();
   std::vector<LibItemSet> incls;
   std::vector<CodeFile*> files;

   for(auto f = fullSet.cbegin(); f != fullSet.cend(); ++f)
   {
      auto file = static_cast<CodeFile*>(*f);
      incls.push_back(file->InclList());
      files.push_back(file);
   }

   //  Create and initialize BUILD, which will contain the set of files that
   //  can be built during the current iteration (LEVEL).  ORDER tracks the
   //  build order for the files in the original set.
   //
   size_t found = 0;
   LibItemSet build;
   BuildOrder order;

   for(size_t level = 0; true; ++level)
   {
      build.clear();

      //  Add a file to BUILD if everything that it #includes has already
      //  been included in the build.  Afterwards, remove it from the list
      //  by nullifying it.
      //
      for(size_t i = 0; i < size; ++i)
      {
         if((files[i] != nullptr) && incls[i].empty())
         {
            auto iter = fileSet.find(files[i]);

            if(iter != fileSet.cend())
            {
               FileLevel item(files[i], level);
               order.push_back(item);
            }

            build.insert(files[i]);
            files[i] = nullptr;
            ++found;
         }
      }

      //  Remove, from every #includes list, all the files that were just
      //  added to the build.  Stop when no more files were added.
      //
      if(!build.empty())
      {
         for(size_t i = 0; i < incls.size(); ++i)
         {
            if(!incls[i].empty())
            {
               SetDifference(incls[i], build);
            }
         }
      }
      else
      {
         break;
      }
   }

   if(found != size)
   {
      //  Some files were not included in the build.
      //
      Debug::SwLog(CodeFileSet_SortInBuildOrder,
         "files not built", files.size() - found);
   }

   std::sort(order.begin(), order.end(), IsSortedByFileLevel);
   return order;
}

//------------------------------------------------------------------------------

void CodeFileSet::to_str(stringVector& strings, bool verbose) const
{
   Debug::ft("CodeFileSet.to_str");

   const auto& fileSet = Items();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = static_cast<CodeFile*>(*f);

      if(verbose)
         strings.push_back(file->Path());
      else
         strings.push_back(file->Name());
   }
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::UsedBy(bool self) const
{
   Debug::ft("CodeFileSet.UsedBy");

   //  Iterate over this set of code files to find what they include.
   //
   const auto& fileSet = Items();
   auto result = new CodeFileSet(TemporaryName(), nullptr);
   auto& usedSet = result->Items();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      //  Count the file as including itself if requested.
      //
      if(self) usedSet.insert(*f);

      auto file = static_cast<CodeFile*>(*f);
      const auto& used = file->InclList();

      for(auto u = used.cbegin(); u != used.cend(); ++u)
      {
         usedSet.insert(*u);
      }
   }

   return result;
}

//------------------------------------------------------------------------------

LibrarySet* CodeFileSet::Users(bool self) const
{
   Debug::ft("CodeFileSet.Users");

   //  Iterate over this set of code files to find those that include them.
   //
   const auto& fileSet = Items();
   auto result = new CodeFileSet(TemporaryName(), nullptr);
   auto& userSet = result->Items();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      //  Count the file as being included by itself if requested.
      //
      if(self) userSet.insert(*f);

      auto file = static_cast<CodeFile*>(*f);
      const auto& users = file->UserList();

      for(auto u = users.cbegin(); u != users.cend(); ++u)
      {
         userSet.insert(*u);
      }
   }

   return result;
}
}
