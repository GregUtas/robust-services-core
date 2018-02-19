//==============================================================================
//
//  CodeFileSet.cpp
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
#include "CodeFileSet.h"
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <sstream>
#include "CodeDir.h"
#include "CodeDirSet.h"
#include "CodeFile.h"
#include "Debug.h"
#include "Formatters.h"
#include "Lexer.h"
#include "Library.h"
#include "NbCliParms.h"
#include "Parser.h"
#include "Registry.h"
#include "SetOperations.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
fn_name CodeFileSet_ctor = "CodeFileSet.ctor";

CodeFileSet::CodeFileSet(const string& name, SetOfIds* set) : CodeSet(name, set)
{
   Debug::ft(CodeFileSet_ctor);
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_dtor = "CodeFileSet.dtor";

CodeFileSet::~CodeFileSet()
{
   Debug::ft(CodeFileSet_dtor);
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_AffectedBy = "CodeFileSet.AffectedBy";

LibrarySet* CodeFileSet::AffectedBy() const
{
   Debug::ft(CodeFileSet_AffectedBy);

   //  What is affected by this set are those that include it, transitively.
   //  Start with the initial set and add files that directly include any
   //  member of the set.
   //
   CodeFileSet* prev = nullptr;
   auto curr = static_cast< CodeFileSet* >(Users(true));
   size_t prevSize = this->Set().size();
   size_t currSize = curr->Set().size();

   //  Keep adding files that #include the new members until the set stops
   //  growing.
   //
   while(prevSize < currSize)
   {
      if(prev != nullptr) prev->Release();
      prev = curr;
      prevSize = currSize;
      curr = static_cast< CodeFileSet* >(prev->Users(true));
      currSize = curr->Set().size();
   }

   if(prev != nullptr) prev->Release();
   return curr;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_Affecters = "CodeFileSet.Affecters";

LibrarySet* CodeFileSet::Affecters() const
{
   Debug::ft(CodeFileSet_Affecters);

   //  What affects this set are what it includes, transitively.  Start with
   //  the initial set and add files that any member directly includes.
   //
   CodeFileSet* prev = nullptr;
   auto curr = static_cast< CodeFileSet* >(UsedBy(true));
   size_t prevSize = this->Set().size();
   size_t currSize = curr->Set().size();

   //  Keep adding files that the new members #include until the set stops
   //  growing.
   //
   while(prevSize < currSize)
   {
      if(prev != nullptr) prev->Release();
      prev = curr;
      prevSize = currSize;
      curr = static_cast< CodeFileSet* >(prev->UsedBy(true));
      currSize = curr->Set().size();
   }

   if(prev != nullptr) prev->Release();
   return curr;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_Check = "CodeFileSet.Check";

word CodeFileSet::Check(ostream* stream, string& expl) const
{
   Debug::ft(CodeFileSet_Check);

   auto& fileSet = Set();

   if(fileSet.empty())
   {
      expl = EmptySet;
      return 0;
   }

   //  In order to generate a report for a file, it must have been parsed.
   //
   auto rc = Parse(expl, "-");
   if(rc != 0) return rc;
   CodeFile::GenerateReport(stream, fileSet);
   return 0;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_CommonAffecters = "CodeFileSet.CommonAffecters";

LibrarySet* CodeFileSet::CommonAffecters() const
{
   Debug::ft(CodeFileSet_CommonAffecters);

   //  The common affecters of this set is the intersection of
   //  each file's affecters.
   //
   auto& fileSet = Set();
   auto result = new CodeFileSet(TemporaryName(), nullptr);
   auto& caSet = result->Set();
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = files.At(*f);
      if(f == fileSet.cbegin())
         SetUnion(caSet, file->Affecters());
      else
         SetIntersection(caSet, file->Affecters());
      if(caSet.empty()) break;
   }

   return result;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_Countlines = "CodeFileSet.Countlines";

word CodeFileSet::Countlines(string& result) const
{
   Debug::ft(CodeFileSet_Countlines);

   result = "linecount: ";

   size_t count = 0;
   auto& fileSet = Set();
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      count += files.At(*f)->GetLexer().LineCount();
   }

   result = result + strInt(count);
   return 0;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_Create = "CodeFileSet.Create";

LibrarySet* CodeFileSet::Create(const string& name, SetOfIds* set) const
{
   Debug::ft(CodeFileSet_Create);

   return new CodeFileSet(name, set);
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_Directories = "CodeFileSet.Directories";

LibrarySet* CodeFileSet::Directories() const
{
   Debug::ft(CodeFileSet_Directories);

   //  Iterate over the set of code files to find their directories.
   //
   auto& fileSet = Set();
   auto result = new CodeDirSet(TemporaryName(), nullptr);
   auto& dirSet = result->Set();
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto d = files.At(*f)->Dir();
      if(d != nullptr) dirSet.insert(d->Did());
   }

   return result;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_FileName = "CodeFileSet.FileName";

LibrarySet* CodeFileSet::FileName(const LibrarySet* that) const
{
   Debug::ft(CodeFileSet_FileName);

   auto& fileSet = Set();
   auto result = new CodeFileSet(TemporaryName(), nullptr);

   //  THAT's name encodes the desired filename (e.g. "Sys").
   //
   auto fn = that->Name();
   if(fn.empty()) return result;

   //  Iterate over the set of code files to find those that begin with "fn".
   //
   auto& fnSet = result->Set();
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      if(files.At(*f)->Name().find(fn) == 0)
      {
         fnSet.insert(*f);
      }
   }

   return result;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_FileType = "CodeFileSet.FileType";

LibrarySet* CodeFileSet::FileType(const LibrarySet* that) const
{
   Debug::ft(CodeFileSet_FileType);

   auto& fileSet = Set();
   auto result = new CodeFileSet(TemporaryName(), nullptr);

   //  THAT's name encodes the desired filetype (e.g. "cpp").
   //
   auto ft = that->Name();
   if(ft.empty()) return result;
   ft = "." + ft;

   //  Iterate over the set of code files to find those that end with ".ft".
   //
   auto& ftSet = result->Set();
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto& name = files.At(*f)->Name();
      auto nsize = name.size();
      auto fsize = ft.size();

      if((nsize > fsize) && (name.rfind(ft) == nsize - fsize))
      {
         ftSet.insert(*f);
      }
   }

   return result;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_Fix = "CodeFileSet.Fix";

word CodeFileSet::Fix(CliThread& cli, std::string& expl) const
{
   Debug::ft(CodeFileSet_Fix);

   auto& fileSet = Set();

   if(fileSet.empty())
   {
      expl = EmptySet;
      return 0;
   }

   //  In order to fix warnings in a file, it must have been checked.
   //
   auto rc = Check(nullptr, expl);
   if(rc != 0) return rc;

   //  Iterate over the set of code files and fix them.
   //
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      rc = files.At(*f)->Fix(cli, expl);
      if(rc != 0) return rc;
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_Format = "CodeFileSet.Format";

word CodeFileSet::Format(string& expl) const
{
   Debug::ft(CodeFileSet_Format);

   auto& fileSet = Set();
   auto& files = Singleton< Library >::Instance()->Files();

   size_t failed = 0;
   size_t changed = 0;
   string err;

   //  Iterate over the set of code files and reformat them.
   //
   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      err.clear();

      auto file = files.At(*f);

      if(file->GetLexer().LineCount() > 0)
      {
         auto rc = file->Format(err);

         if(rc != 0)
         {
            if(rc < 0)
               ++failed;
            else
               ++changed;
         }

         Debug::Progress((rc >= 0 ? EMPTY_STR : " ERROR: " + err), true, true);
      }
   }

   std::ostringstream summary;
   summary << "Total: " << fileSet.size() << ", changed: " << changed;
   if(failed > 0) summary << ", failed: " << failed;
   expl += summary.str();
   return 0;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_FoundIn = "CodeFileSet.FoundIn";

LibrarySet* CodeFileSet::FoundIn(const LibrarySet* that) const
{
   Debug::ft(CodeFileSet_FoundIn);

   //  Iterate over the set of code files to find those which appear
   //  in one of THAT's directories.
   //
   auto& dirSet = static_cast< const CodeDirSet* >(that)->Set();
   auto& fileSet = Set();
   auto result = new CodeFileSet(TemporaryName(), nullptr);
   auto& foundSet = result->Set();
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto d = files.At(*f)->Dir();

      if(d != nullptr)
      {
         SetOfIds::const_iterator it = dirSet.find(d->Did());
         if(it != dirSet.cend()) foundSet.insert(*f);
      }
   }

   return result;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_Implements = "CodeFileSet.Implements";

LibrarySet* CodeFileSet::Implements() const
{
   Debug::ft(CodeFileSet_Implements);

   //  In order to find where something declared in a file is defined, and
   //  vice versa, everything that affects the file, and that is affected
   //  by it, must have been parsed.
   //
   auto abSet = static_cast< CodeFileSet* >(this->AffectedBy());
   auto asSet = static_cast< CodeFileSet* >(this->Affecters());
   auto parseSet = new SetOfIds;
   SetUnion(*parseSet, abSet->Set(), asSet->Set());

   string expl;
   auto parseFiles = new CodeFileSet(TemporaryName(), parseSet);
   parseFiles->Parse(expl, "-");
   abSet->Release();
   asSet->Release();
   parseFiles->Release();

   //  Iterate over the set of code files, adding files that implement ones
   //  already in the set.
   //
   auto& fileSet = Set();
   auto result = new CodeFileSet(TemporaryName(), nullptr);
   auto& imSet = result->Set();
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = files.At(*f);
      SetUnion(imSet, file->Implementers());
   }

   return result;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_List = "CodeFileSet.List";

word CodeFileSet::List(ostream& stream, string& expl) const
{
   Debug::ft(CodeFileSet_List);

   auto& fileSet = Set();

   if(fileSet.empty())
   {
      stream << spaces(2) << EmptySet << CRLF;
      return 0;
   }

   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      stream << spaces(2) << files.At(*f)->FullName() << CRLF;
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_MatchString = "CodeFileSet.MatchString";

LibrarySet* CodeFileSet::MatchString(const LibrarySet* that) const
{
   Debug::ft(CodeFileSet_MatchString);

   auto& fileSet = Set();
   auto result = new CodeFileSet(TemporaryName(), nullptr);

   //  THAT's name is the string to be searched for.
   //
   auto s = that->Name();
   if(s.empty()) return result;

   //  Iterate over the set of code files to find those that contains S.
   //
   auto& msSet = result->Set();
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = files.At(*f);
      auto code = file->GetCode();
      if(code == nullptr) continue;
      if(code->find(s) != string::npos) msSet.insert(*f);
   }

   return result;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_NeededBy = "CodeFileSet.NeededBy";

LibrarySet* CodeFileSet::NeededBy() const
{
   Debug::ft(CodeFileSet_NeededBy);

   //  The code files needed by the set fs1 are those that must also appear
   //  in a build which contains fs1 in order to resolve all symbols during
   //  linking.  That set is found by repeating >assign fs1 im as fs1 until
   //  the set stops growing.
   //
   size_t prevCount = 0;
   size_t currCount = Set().size();
   LibrarySet* nbSet = (LibrarySet*) this;
   LibrarySet* asSet = nullptr;

   while(prevCount < currCount)
   {
      asSet = nbSet->Affecters();
      if(nbSet != this) nbSet->Release();
      nbSet = asSet->Implements();
      if(nbSet == nullptr) return nbSet;
      asSet->Release();
      prevCount = currCount;
      currCount = static_cast< CodeFileSet* >(nbSet)->Set().size();
   }

   return nbSet;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_Needers = "CodeFileSet.Needers";

LibrarySet* CodeFileSet::Needers() const
{
   Debug::ft(CodeFileSet_Needers);

   //  The code files that need any in set fs1 are those that could not appear
   //  in a build without including fs1 to resolve all symbols during linking.
   //  That set of files is found by repeating >assign fs1 im ab fs1 until the
   //  set stops growing.
   //
   size_t prevCount = 0;
   size_t currCount = Set().size();
   LibrarySet* nsSet = (LibrarySet*) this;
   LibrarySet* abSet = nullptr;

   while(prevCount < currCount)
   {
      abSet = nsSet->AffectedBy();
      if(nsSet != this) nsSet->Release();
      nsSet = abSet->Implements();
      if(nsSet == nullptr) return nsSet;
      abSet->Release();
      prevCount = currCount;
      currCount = static_cast< CodeFileSet* >(nsSet)->Set().size();
   }

   return nsSet;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_Parse = "CodeFileSet.Parse";

word CodeFileSet::Parse(string& expl, const string& opts) const
{
   Debug::ft(CodeFileSet_Parse);

   auto& fileSet = Set();

   if(fileSet.empty())
   {
      expl = EmptySet;
      return 0;
   }

   //  Create a copy of the files to be parsed.  Include files that affect them,
   //  along with substitute files.  Calculate the build order of the resulting
   //  set.  Parse substitute files first, followed by the .h's.  This allows
   //  the symbols visible to a file to be determined before parsing it.  Using
   //  declarations in #included files affect visibility, so an #included file
   //  must already have been parsed.
   //
   auto library = Singleton< Library >::Instance();
   auto parseSet = new SetOfIds(fileSet);
   SetUnion(*parseSet, library->SubsFiles()->Set());

   auto parseFiles = new CodeFileSet(TemporaryName(), parseSet);
   auto affects = parseFiles->Affecters();
   auto order = static_cast< CodeFileSet* >(affects)->SortInBuildOrder();

   auto& files = library->Files();
   auto parser = std::unique_ptr< Parser >(new Parser(opts));
   size_t total = 0;
   size_t failed = 0;

   for(auto f = order->cbegin(); f != order->cend(); ++f)
   {
      auto file = files.At(f->fid);

      if(file->IsSubsFile())
      {
         if(!parser->Parse(*file)) ++failed;
         ++total;
      }
   }

   for(auto f = order->cbegin(); f != order->cend(); ++f)
   {
      auto file = files.At(f->fid);

      if(file->IsHeader() && (file->GetLexer().LineCount() > 0))
      {
         if(!parser->Parse(*file)) ++failed;
         ++total;
      }
   }

   for(auto f = order->cbegin(); f != order->cend(); ++f)
   {
      auto file = files.At(f->fid);

      if(file->IsCpp())
      {
         SetOfIds::const_iterator it = fileSet.find(f->fid);

         if(it != fileSet.cend())
         {
            if(!parser->Parse(*file)) ++failed;
            ++total;
         }
      }
   }

   parser.reset();
   parseFiles->Release();
   affects->Release();

   std::ostringstream summary;
   summary << "Total=" << total << ", failed=" << failed;
   expl = summary.str();
   return 0;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_Scan = "CodeFileSet.Scan";

word CodeFileSet::Scan
   (ostream& stream, const string& pattern, string& expl) const
{
   Debug::ft(CodeFileSet_Scan);

   auto& fileSet = Set();

   if(fileSet.empty())
   {
      stream << EmptySet << CRLF;
      return 0;
   }

   stream << "Searching for \"" << pattern << QUOTE << CRLF;

   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      auto file = files.At(*f);
      auto code = file->GetCode();
      if(code == nullptr) continue;

      auto pos = code->find(pattern);
      auto shown = false;

      while(pos != string::npos)
      {
         if(!shown)
         {
            stream << file->FullName() << ':' << CRLF;
            shown = true;
         }

         auto line = file->GetLexer().GetLineNum(pos);
         auto str = file->GetLexer().GetNthLine(line);

         stream << spaces(2) << line + 1 << ": " << str << CRLF;
         pos = code->find(pattern, pos + pattern.size());
      }
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_Show = "CodeFileSet.Show";

word CodeFileSet::Show(string& result) const
{
   Debug::ft(CodeFileSet_Show);

   auto& fileSet = Set();
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      result = result + files.At(*f)->Name() + ", ";
   }

   return Shown(result);
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_Sort = "CodeFileSet.Sort";

word CodeFileSet::Sort(ostream& stream, string& expl) const
{
   Debug::ft(CodeFileSet_Sort);

   //  Get the build order.
   //
   auto order = SortInBuildOrder();

   if(order->empty())
   {
      expl = EmptySet;
      return 0;
   }

   //  List the files in the original set, showing their build level.
   //
   auto heading = false;
   int room = 65;

   auto fileSet = Set();
   auto& files = Singleton< Library >::Instance()->Files();
   size_t shown = 0;
   size_t level = order->front().level + 1;  // to cause mismatch

   for(auto f = order->cbegin(); f != order->cend(); ++f)
   {
      //  List the files that were just included in the build.  Limit
      //  each line to 80 characters.
      //
      if(f->level != level)
      {
         level = f->level;
         heading = true;
         if(shown > 0) stream << CRLF;
      }

      if(fileSet.find(f->fid) == fileSet.cend()) continue;

      if(heading)
      {
         stream << "LEVEL " << level << ':';
         heading = false;
         room = 69;
      }
      else
      {
         stream << ',';
      }

      auto& name = files.At(f->fid)->Name();
      int size = name.size();

      if(room - size < 2)
      {
         stream << CRLF;
         stream << spaces(2) << name;
         room = 78 - (size + 2);
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

fn_name CodeFileSet_SortInBuildOrder = "CodeFileSet.SortInBuildOrder";

BuildOrderPtr CodeFileSet::SortInBuildOrder() const
{
   Debug::ft(CodeFileSet_SortInBuildOrder);

   //  Create the array INCLS and clone every file's #include list into it.
   //  The parallel array FIDS contains the file identifier associated with
   //  each entry in INCLS.
   //
   auto fileSet = Set();
   auto& files = Singleton< Library >::Instance()->Files();
   auto size = files.Size();
   auto incls = std::unique_ptr< SetOfIds[] >(new SetOfIds[size]);
   auto fids = std::unique_ptr< id_t[] >(new id_t[size]);
   size_t n = 0;

   for(CodeFile* f = files.First(); f != nullptr; files.Next(f))
   {
      if(f != nullptr)
      {
         incls[n] = f->InclList();
         fids[n] = f->Fid();
      }

      ++n;
   }

   //  Create and initialize BUILD, which will contain the set of files that
   //  can be built during the current iteration (LEVEL).  ORDER tracks the
   //  build order for the files in the original set.
   //
   size_t found = 0;
   SetOfIds build;
   FileLevel item;

   BuildOrderPtr order(new BuildOrder);

   for(size_t level = 0; true; ++level)
   {
      build.clear();

      //  Add a file to BUILD if everything that it #includes has already
      //  been included in the build.  Afterwards, remove it from the list
      //  by nullifying its identifier.
      //
      for(size_t i = 0; i < size; ++i)
      {
         if((fids[i] != NIL_ID) && incls[i].empty())
         {
            SetOfIds::const_iterator it = fileSet.find(fids[i]);

            if(it != fileSet.cend())
            {
               item.fid = fids[i];
               item.level = level;
               order->push_back(item);
            }

            build.insert(fids[i]);
            fids[i] = NIL_ID;
            ++found;
         }
      }

      //  Stop if no more files could be built.  This should only occur
      //  after all files have been built.
      //
      if(build.empty())
      {
         if(found != size)
         {
            Debug::SwErr(CodeFileSet_SortInBuildOrder, found, size);
         }
         break;
      }

      //  Remove, from every #includes list, all of the files that were
      //  just included in the build.
      //
      for(size_t i = 0; i < size; ++i)
      {
         if(!incls[i].empty())
         {
            SetDifference(incls[i], build);
         }
      }
   }

   return order;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_Trim = "CodeFileSet.Trim";

word CodeFileSet::Trim(ostream& stream, string& expl) const
{
   Debug::ft(CodeFileSet_Trim);

   if(Set().empty())
   {
      expl = EmptySet;
      return 0;
   }

   //  In order to trim a file, it must have been parsed.  Trim headers
   //  in build order so that the recommendations for files built later
   //  can take into account recommendations for headers built earlier.
   //
   auto rc = Parse(expl, "-");
   if(rc != 0) return rc;

   auto& files = Singleton< Library >::Instance()->Files();

   auto order = SortInBuildOrder();

   for(auto f = order->cbegin(); f != order->cend(); ++f)
   {
      auto file = files.At(f->fid);
      if(file->IsHeader()) file->Trim(&stream);
   }

   for(auto f = order->cbegin(); f != order->cend(); ++f)
   {
      auto file = files.At(f->fid);
      if(file->IsCpp()) file->Trim(&stream);
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_UsedBy = "CodeFileSet.UsedBy";

LibrarySet* CodeFileSet::UsedBy(bool self) const
{
   Debug::ft(CodeFileSet_UsedBy);

   //  Iterate over this set of code files to find what they include.
   //
   auto& fileSet = Set();
   auto result = new CodeFileSet(TemporaryName(), nullptr);
   auto& usedSet = result->Set();
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      //  Count the file as including itself if requested.
      //
      if(self) usedSet.insert(*f);

      auto& used = files.At(*f)->InclList();

      for(auto u = used.cbegin(); u != used.cend(); ++u)
      {
         usedSet.insert(*u);
      }
   }

   return result;
}

//------------------------------------------------------------------------------

fn_name CodeFileSet_Users = "CodeFileSet.Users";

LibrarySet* CodeFileSet::Users(bool self) const
{
   Debug::ft(CodeFileSet_Users);

   //  Iterate over this set of code files to find those that include them.
   //
   auto& fileSet = Set();
   auto result = new CodeFileSet(TemporaryName(), nullptr);
   auto& userSet = result->Set();
   auto& files = Singleton< Library >::Instance()->Files();

   for(auto f = fileSet.cbegin(); f != fileSet.cend(); ++f)
   {
      //  Count the file as being included by itself if requested.
      //
      if(self) userSet.insert(*f);

      auto& users = files.At(*f)->UserList();

      for(auto u = users.cbegin(); u != users.cend(); ++u)
      {
         userSet.insert(*u);
      }
   }

   return result;
}
}
