//==============================================================================
//
//  Library.cpp
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
#include "Library.h"
#include <memory>
#include <sstream>
#include "CfgParmRegistry.h"
#include "CfgStrParm.h"
#include "CodeDir.h"
#include "CodeDirSet.h"
#include "CodeFile.h"
#include "CodeFileSet.h"
#include "CodeTypes.h"
#include "CxxArea.h"
#include "CxxFwd.h"
#include "CxxRoot.h"
#include "CxxString.h"
#include "Debug.h"
#include "Formatters.h"
#include "Interpreter.h"
#include "LibraryTypes.h"
#include "LibraryVarSet.h"
#include "NbCliParms.h"
#include "Singleton.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Names for pre-defined library variables.
//
fixed_string DirsStr  = "$dirs";
fixed_string FilesStr = "$files";
fixed_string HdrsStr  = "$hdrs";
fixed_string CppsStr  = "$cpps";
fixed_string ExtsStr  = "$exts";
fixed_string SubsStr  = "$subs";
fixed_string VarsStr  = "$vars";

//------------------------------------------------------------------------------

string Library::SourcePath_ = EMPTY_STR;
const size_t Library::MaxDirs = 500;
const size_t Library::MaxFiles = 30000;
fixed_string Library::SubsDir = "subs";

//------------------------------------------------------------------------------

fn_name Library_ctor = "Library.ctor";

Library::Library() :
   sourcePathCfg_(nullptr),
   dirSet_(nullptr),
   fileSet_(nullptr),
   hdrSet_(nullptr),
   cppSet_(nullptr),
   extSet_(nullptr),
   subsSet_(nullptr),
   varSet_(nullptr)
{
   Debug::ft(Library_ctor);

   dirs_.Init(MaxDirs, CodeDir::CellDiff(), MemPerm);
   files_.Init(MaxFiles, CodeFile::CellDiff(), MemPerm);
   vars_.Init(LibrarySet::LinkDiff());

   //  After a restart, sourcePathCfg_ may still exist, so try to look it
   //  up before creating it.
   //
   auto reg = Singleton< CfgParmRegistry >::Instance();

   sourcePathCfg_.reset
      (static_cast< CfgStrParm* >(reg->FindParm("SourcePath")));

   if(sourcePathCfg_ == nullptr)
   {
      sourcePathCfg_.reset(new CfgStrParm
         ("SourcePath", EMPTY_STR, &SourcePath_, "source code directory"));
      reg->BindParm(*sourcePathCfg_);
   }
}

//------------------------------------------------------------------------------

fn_name Library_dtor = "Library.dtor";

Library::~Library()
{
   Debug::ft(Library_dtor);

   //  When dirs_, files_, and vars_ are deleted, so are their elements.
}

//------------------------------------------------------------------------------

fn_name Library_AddFile = "Library.AddFile";

void Library::AddFile(CodeFile& file)
{
   Debug::ft(Library_AddFile);

   //  Add the file to the file registry, $files, either $hdrs or $cpps,
   //  $exts if it is external, and $subs if it declares external items.
   //
   files_.Insert(file);

   auto fid = file.Fid();
   fileSet_->Set().insert(fid);

   if(file.IsHeader())
   {
      hdrSet_->Set().insert(fid);
      if(file.IsSubsFile()) subsSet_->Set().insert(fid);
   }
   else
   {
      cppSet_->Set().insert(fid);
   }

   if(file.IsExtFile()) extSet_->Set().insert(fid);
}

//------------------------------------------------------------------------------

fn_name Library_AddVar = "Library.AddVar";

void Library::AddVar(LibrarySet& var)
{
   Debug::ft(Library_AddVar);

   vars_.Enq(var);
}

//------------------------------------------------------------------------------

fn_name Library_Assign = "Library.Assign";

word Library::Assign
   (const string& name, const string& expr, size_t pos, string& expl)
{
   Debug::ft(Library_Assign);

   //  If V is an existing variable, it must not be read-only.
   //
   auto v = FindVar(name);

   if(v != nullptr)
   {
      if(v->IsReadOnly())
      {
         expl = "That variable is read-only.";
         return -4;
      }
   }
   else
   {
      //  V isn't an existing variable.  Ensure that it's not the name
      //  of an existing directory, file, or operator.
      //
      auto d = FindDir(name);

      if(d != nullptr)
      {
         expl = "That variable is already assigned to a directory.";
         return -4;
      }

      auto f = FindFile(name);

      if(f != nullptr)
      {
         expl = "That variable is the name of a code file.";
         return -4;
      }

      if(Interpreter::IsOperator(name))
      {
         expl = "That variable is the name of an operator.";
         return -4;
      }
   }

   //  Evaluate the expression and ensure that the result is something
   //  the can be assigned to a variable.
   //
   auto s = Evaluate(expr, pos);

   if(s == nullptr)
   {
      expl = AllocationError;
      return -7;
   }

   auto rc = s->PreAssign(expl);

   if(rc != 0)
   {
      s->Release();
      return rc;
   }

   //  If the variable already exists but its type will change, it must
   //  be deleted so that the proper subclass can be created.
   //
   if(v != nullptr)
   {
      if(v->GetType() != s->GetType())
      {
         delete v;
         v = nullptr;
      }
   }

   //  Create the variable if it does not exist.
   //
   if(v == nullptr)
   {
      v = s->Create(name, nullptr);

      if(v == nullptr)
      {
         expl = AllocationError;
         return -7;
      }
   }

   //  Finally, assign the variable its value.
   //
   if(v->Assign(s) == nullptr)
   {
      expl = AllocationError;
      return -7;
   }

   expl = SuccessExpl;
   return 0;
}

//------------------------------------------------------------------------------

void Library::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Base::Display(stream, prefix, options);

   stream << prefix << "SourcePath    : " << SourcePath_ << CRLF;
   stream << prefix << "sourcePathCfg : " << sourcePathCfg_.get() << CRLF;
   stream << prefix << "dirs : " << CRLF;
   dirs_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "files : " << CRLF;
   files_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "vars : " << CRLF;
   vars_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name Library_EnsureFile = "Library.EnsureFile";

CodeFile* Library::EnsureFile(const string& file, CodeDir* dir)
{
   Debug::ft(Library_EnsureFile);

   //  If FILE was taken from an #include, remove any path.
   //
   auto name = file;
   auto pos = name.rfind('/');
   if(pos != string::npos) name = name.substr(pos + 1);
   auto f = FindFile(name);

   if(f != nullptr)
   {
      if(dir != nullptr)
      {
         if(f->Dir() == nullptr)
         {
            //  Now we know FILE's directory.
            //
            f->SetDir(dir);
            extSet_->Set().erase(f->Fid());
            if(f->IsSubsFile()) subsSet_->Set().insert(f->Fid());
         }
         else if(f->Dir() != dir)
         {
            //c The same filename in different directories is not supported.
            //
            Debug::SwLog(Library_EnsureFile, file, f->Fid());
            return nullptr;
         }
      }

      return f;
   }

   return new CodeFile(name, dir);
}

//------------------------------------------------------------------------------

fn_name Library_EnsureVar = "Library.EnsureVar";

LibrarySet* Library::EnsureVar(const string& s) const
{
   Debug::ft(Library_EnsureVar);

   //  If S is a variable, return it.  If it is the name of a directory
   //  or file, create a single-member set for it and return it.
   //
   auto set = FindVar(s);

   if(set != nullptr) return set;

   auto d = FindDir(s);

   if(d != nullptr)
   {
      string name = LibrarySet::TemporaryChar + d->Name();
      auto result = new CodeDirSet(name, nullptr);
      auto& dirSet = result->Set();
      dirSet.insert(d->Did());
      return result;
   }

   auto f = FindFile(s);

   if(f != nullptr)
   {
      string name = LibrarySet::TemporaryChar + f->Name();
      auto result = new CodeFileSet(name, nullptr);
      auto& fileSet = result->Set();
      fileSet.insert(f->Fid());
      return result;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Library_EraseVar = "Library.EraseVar";

void Library::EraseVar(LibrarySet& var)
{
   Debug::ft(Library_EraseVar);

   vars_.Exq(var);
}

//------------------------------------------------------------------------------

fn_name Library_Evaluate = "Library.Evaluate";

LibrarySet* Library::Evaluate(const string& expr, size_t pos) const
{
   Debug::ft(Library_Evaluate);

   //  Purge any temporary variables that were not released when evaluating
   //  the previous expression.
   //
   for(auto curr = vars_.First(); curr != nullptr; NO_OP)
   {
      auto next = vars_.Next(*curr);
      curr->Release();
      curr = next;
   }

   auto interpreter =
      std::unique_ptr< Interpreter >(new Interpreter(expr, pos));
   auto set = interpreter->Evaluate();
   interpreter.reset();
   return set;
}

//------------------------------------------------------------------------------

fn_name Library_Export = "Library.Export";

word Library::Export(ostream& stream, const string& opts) const
{
   Debug::ft(Library_Export);

   if(opts.find(NamespaceView) != string::npos)
   {
      auto root = Singleton< CxxRoot >::Instance();
      auto gns = root->GlobalNamespace();
      auto options = Flags(FQ_Mask | NS_Mask);
      if(opts.find(ItemStatistics) != string::npos) options.set(DispStats);

      stream << "NAMESPACE VIEW" << CRLF << CRLF;
      root->Display(stream, EMPTY_STR, options);
      gns->Display(stream, EMPTY_STR, options);
      stream << string(80, '=') << CRLF;
   }

   auto rule = false;

   if((opts.find(CanonicalFileView) != string::npos) ||
      (opts.find(OriginalFileView) != string::npos))
   {
      stream << "FILE VIEW" << CRLF << CRLF;

      for(auto f = files_.First(); f != nullptr; files_.Next(f))
      {
         f->DisplayItems(stream, opts);
      }

      rule = true;
   }

   if(opts.find(ClassHierarchyView) != string::npos)
   {
      if(rule) stream << string(80, '=') << CRLF;
      stream << "CLASS VIEW" << CRLF << CRLF;

      ClassVector roots;

      for(auto f = files_.First(); f != nullptr; files_.Next(f))
      {
         auto classes = f->Classes();

         for(auto c = classes->cbegin(); c != classes->cend(); ++c)
         {
            if((*c)->BaseClass() == nullptr) roots.push_back(*c);
         }
      }

      for(auto c = roots.cbegin(); c != roots.cend(); ++c)
      {
         (*c)->DisplayHierarchy(stream, EMPTY_STR);
      }
   }

   return 0;
}

//------------------------------------------------------------------------------

fn_name Library_FindDir = "Library.FindDir";

CodeDir* Library::FindDir(const string& name) const
{
   Debug::ft(Library_FindDir);

   for(auto d = dirs_.First(); d != nullptr; dirs_.Next(d))
   {
      if(d->Name() == name) return d;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Library_FindFile = "Library.FindFile";

CodeFile* Library::FindFile(const string& name) const
{
   Debug::ft(Library_FindFile);

   //  Case is ignored in source code file names, so convert NAME and
   //  candidate file names to lower case before comparing them.
   //
   auto key = strLower(GetFileName(name));

   for(auto f = files_.First(); f != nullptr; files_.Next(f))
   {
      if(strLower(f->Name()) == key) return f;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Library_FindVar = "Library.FindVar";

LibrarySet* Library::FindVar(const string& name) const
{
   Debug::ft(Library_FindVar);

   //  Exclude temporary variables from the search.  The are currently no
   //  situations where including them is useful, and excluding them avoids
   //  the case where an expression tries to create two temporary variables
   //  with the same name.  If the second attempt simply returns the first
   //  variable, a trap occurs if, for example, one operator releases the
   //  variable but a second operator also tries to use it.
   //
   for(auto v = vars_.First(); v != nullptr; vars_.Next(v))
   {
      if(!v->IsTemporary() && (v->Name() == name)) return v;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Library_Import = "Library.Import";

word Library::Import(const string& name, const string& path, string& expl)
{
   Debug::ft(Library_Import);

   for(auto d = dirs_.First(); d != nullptr; dirs_.Next(d))
   {
      //  Both DIR and PATH should be new, but don't complain if they're
      //  already registered together.
      //
      auto dirExists = (d->Name() == name);
      auto pathExists = (d->Path() == path);

      if(dirExists && pathExists)
      {
         expl = "This directory and path already exist.";
         return 0;
      }

      if(dirExists)
      {
         std::ostringstream stream;

         stream << "Directory " << name << " already exists for "
            << d->Path() << '.';
         expl = stream.str();
         return -1;
      }

      if(pathExists)
      {
         std::ostringstream stream;

         stream << path << " already exists for directory "
            << d->Name() << '.';
         expl = stream.str();
         return -1;
      }
   }

   //  Create a new directory and extract all of its code files.
   //  On success, add the directory to $dirs, else delete it.
   //
   auto dir = new CodeDir(name, path);

   if(dir != nullptr)
   {
      dirs_.Insert(*dir);
      auto rc = dir->Extract(expl);

      if(rc == 0)
      {
         dirSet_->Set().insert(dir->Did());
      }
      else
      {
         dirs_.Erase(*dir);
         delete dir;
      }

      return rc;
   }

   expl = AllocationError;
   return -1;
}

//------------------------------------------------------------------------------

fn_name Library_Purge = "Library.Purge";

word Library::Purge(const string& name, string& expl) const
{
   Debug::ft(Library_Purge);

   //  If V is an existing variable, it must not be read-only.
   //
   auto v = FindVar(name);

   if(v != nullptr)
   {
      if(v->IsReadOnly())
      {
         expl = "That variable is read-only.";
         return -4;
      }

      delete v;
   }

   expl = SuccessExpl;
   return 0;
}

//------------------------------------------------------------------------------

fn_name Library_Shutdown = "Library.Shutdown";

void Library::Shutdown(RestartLevel level)
{
   Debug::ft(Library_Shutdown);

   //  The library is now preserved during restarts.
   //
   if(level < RestartReboot) return;

   //  Delete variables, files, and directories.
   //
   vars_.Purge();
   files_.Purge();
   dirs_.Purge();
}

//------------------------------------------------------------------------------

fn_name Library_Startup = "Library.Startup";

void Library::Startup(RestartLevel level)
{
   Debug::ft(Library_Startup);

   //  The library is now preserved during restarts.
   //
   if(level < RestartReboot) return;

   //  Create the fixed sets.
   //
   auto set = new SetOfIds;
   dirSet_ = new CodeDirSet(DirsStr, set);

   set = new SetOfIds;
   fileSet_ = new CodeFileSet(FilesStr, set);

   set = new SetOfIds;
   hdrSet_ = new CodeFileSet(HdrsStr, set);

   set = new SetOfIds;
   cppSet_ = new CodeFileSet(CppsStr, set);

   set = new SetOfIds;
   extSet_ = new CodeFileSet(ExtsStr, set);

   set = new SetOfIds;
   subsSet_ = new CodeFileSet(SubsStr, set);

   varSet_ = new LibraryVarSet(VarsStr);
}
}
