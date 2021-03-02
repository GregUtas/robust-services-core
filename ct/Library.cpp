//==============================================================================
//
//  Library.cpp
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
#include "Library.h"
#include <iterator>
#include <set>
#include <sstream>
#include "CfgParmRegistry.h"
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
#include "FunctionGuard.h"
#include "Interpreter.h"
#include "LibraryVarSet.h"
#include "NbCliParms.h"
#include "Restart.h"
#include "Singleton.h"
#include "ThisThread.h"

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

fixed_string Library::SubsDir = "subs";

//------------------------------------------------------------------------------

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
   Debug::ft("Library.ctor");

   sourcePathCfg_.reset(new CfgStrParm
      ("SourcePath", EMPTY_STR, "source code directory"));
   Singleton< CfgParmRegistry >::Instance()->BindParm(*sourcePathCfg_);
}

//------------------------------------------------------------------------------

Library::~Library()
{
   Debug::ftnt("Library.dtor");

   //  When dirs_ and files_ are deleted, so are their elements.
   //
   for(auto v = vars_.cbegin(); v != vars_.cend(); ++v)
   {
      delete *v;
   }
}

//------------------------------------------------------------------------------

void Library::AddFile(CodeFile& file)
{
   Debug::ft("Library.AddFile");

   //  Add the file to the file registry, $files, either $hdrs or $cpps,
   //  $exts if it is unknown, and $subs if it declares external items.
   //
   files_.push_back(CodeFilePtr(&file));
   fileSet_->Items().insert(&file);

   if(file.IsHeader())
   {
      hdrSet_->Items().insert(&file);
      if(file.IsSubsFile()) subsSet_->Items().insert(&file);
   }
   else
   {
      cppSet_->Items().insert(&file);
   }

   if(file.IsExtFile()) extSet_->Items().insert(&file);
}

//------------------------------------------------------------------------------

void Library::AddVar(LibrarySet& var)
{
   Debug::ft("Library.AddVar");

   vars_.push_back(&var);
}

//------------------------------------------------------------------------------

word Library::Assign
   (const string& name, const string& expr, size_t pos, string& expl)
{
   Debug::ft("Library.Assign");

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

   stream << prefix << "sourcePathCfg : " << sourcePathCfg_.get() << CRLF;

   stream << prefix << "dirs : " << CRLF;
   for(auto d = dirs_.cbegin(); d != dirs_.cend(); ++d)
   {
      (*d)->Display(stream, prefix + spaces(2), options);
   }

   stream << prefix << "files : " << CRLF;
   for(auto f = files_.cbegin(); f != files_.cend(); ++f)
   {
      (*f)->Display(stream, prefix + spaces(2), options);
   }

   stream << prefix << "vars : " << CRLF;
   for(auto v = vars_.cbegin(); v != vars_.cend(); ++v)
   {
      (*v)->Display(stream, prefix + spaces(2), options);
   }
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
            extSet_->Items().erase(f);
            if(f->IsSubsFile()) subsSet_->Items().insert(f);
         }
         else if(f->Dir() != dir)
         {
            //c The same filename in different directories is not supported.
            //
            Debug::SwLog(Library_EnsureFile, file, 0);
            return nullptr;
         }
      }

      return f;
   }

   return new CodeFile(name, dir);
}

//------------------------------------------------------------------------------

LibrarySet* Library::EnsureVar(const string& s) const
{
   Debug::ft("Library.EnsureVar");

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
      auto& dirSet = result->Items();
      dirSet.insert(d);
      return result;
   }

   auto f = FindFile(s);

   if(f != nullptr)
   {
      string name = LibrarySet::TemporaryChar + f->Name();
      auto result = new CodeFileSet(name, nullptr);
      auto& fileSet = result->Items();
      fileSet.insert(f);
      return result;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void Library::EraseVar(const LibrarySet* var)
{
   Debug::ftnt("Library.EraseVar");

   for(auto v = vars_.cbegin(); v != vars_.cend(); ++v)
   {
      if(*v == var)
      {
         vars_.erase(v);
         return;
      }
   }
}

//------------------------------------------------------------------------------

LibrarySet* Library::Evaluate(const string& expr, size_t pos) const
{
   Debug::ft("Library.Evaluate");

   //  Purge any temporary variables that were not released when evaluating
   //  the previous expression.
   //
   for(auto curr = vars_.begin(); curr != vars_.end(); NO_OP)
   {
      auto next = std::next(curr);
      (*curr)->Release();
      curr = next;
   }

   std::unique_ptr< Interpreter > interpreter(new Interpreter(expr, pos));
   auto set = interpreter->Evaluate();
   interpreter.reset();
   return set;
}

//------------------------------------------------------------------------------

void Library::Export(ostream& stream, const string& opts) const
{
   Debug::ft("Library.Export");

   if(opts.find(NamespaceView) != string::npos)
   {
      auto root = Singleton< CxxRoot >::Instance();
      auto gns = root->GlobalNamespace();
      Flags options(FQ_Mask | NS_Mask);
      if(opts.find(ItemStatistics) != string::npos) options.set(DispStats);

      stream << "NAMESPACE VIEW" << CRLF << CRLF;
      root->Display(stream, EMPTY_STR, options);
      gns->Display(stream, EMPTY_STR, options);
      stream << string(LINE_LENGTH_MAX, '=') << CRLF;
   }

   auto rule = false;

   if((opts.find(CanonicalFileView) != string::npos) ||
      (opts.find(OriginalFileView) != string::npos))
   {
      stream << "FILE VIEW" << CRLF << CRLF;

      for(auto f = files_.cbegin(); f != files_.cend(); ++f)
      {
         (*f)->DisplayItems(stream, opts);
      }

      rule = true;
   }

   if(opts.find(ClassHierarchyView) != string::npos)
   {
      if(rule) stream << string(LINE_LENGTH_MAX, '=') << CRLF;
      stream << "CLASS VIEW" << CRLF << CRLF;

      ClassVector roots;

      for(auto f = files_.cbegin(); f != files_.cend(); ++f)
      {
         auto classes = (*f)->Classes();

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
}

//------------------------------------------------------------------------------

CodeDir* Library::FindDir(const string& name) const
{
   Debug::ft("Library.FindDir");

   for(auto d = dirs_.cbegin(); d != dirs_.cend(); ++d)
   {
      if((*d)->Name() == name) return d->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

CodeFile* Library::FindFile(const string& name) const
{
   Debug::ft("Library.FindFile");

   //  Case is ignored in source code file names, so convert NAME and
   //  candidate file names to lower case before comparing them.
   //
   auto key = strLower(GetFileName(name));

   for(auto f = files_.cbegin(); f != files_.cend(); ++f)
   {
      if(strLower((*f)->Name()) == key) return f->get();
   }

   return nullptr;
}

//------------------------------------------------------------------------------

LibrarySet* Library::FindVar(const string& name) const
{
   Debug::ft("Library.FindVar");

   //  Exclude temporary variables from the search.  There are currently no
   //  situations where including them is useful, and excluding them avoids
   //  the case where an expression tries to create two temporary variables
   //  with the same name.  If the second attempt simply returns the first
   //  variable, a trap occurs if, for example, one operator releases the
   //  variable but a second operator also tries to use it.
   //
   for(auto v = vars_.cbegin(); v != vars_.cend(); ++v)
   {
      if(!(*v)->IsTemporary() && ((*v)->Name() == name)) return *v;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

word Library::Import(const string& name, const string& path, string& expl)
{
   Debug::ft("Library.Import");

   for(auto d = dirs_.cbegin(); d != dirs_.cend(); ++d)
   {
      //  Both DIR and PATH should be new, but don't complain if they're
      //  already registered together.
      //
      auto dirExists = ((*d)->Name() == name);
      auto pathExists = ((*d)->Path() == path);

      if(dirExists && pathExists)
      {
         expl = "This directory and path already exist.";
         return 0;
      }

      if(dirExists)
      {
         std::ostringstream stream;

         stream << "Directory " << name << " already exists for "
            << (*d)->Path() << '.';
         expl = stream.str();
         return -1;
      }

      if(pathExists)
      {
         std::ostringstream stream;

         stream << path << " already exists for directory "
            << (*d)->Name() << '.';
         expl = stream.str();
         return -1;
      }
   }

   //  Create a new directory and extract all of its code files.
   //  On success, add the directory to $dirs, else delete it.
   //
   auto dir = new CodeDir(name, path);
   dirs_.push_back(CodeDirPtr(dir));

   auto rc = dir->Extract(expl);

   if(rc == 0)
   {
      dirSet_->Items().insert(dir);
   }
   else
   {
      dirs_.pop_back();
   }

   return rc;
}

//------------------------------------------------------------------------------

word Library::Purge(const string& name, string& expl)
{
   Debug::ft("Library.Purge");

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

void Library::Shrink()
{
   Debug::ft("Library.Shrink");

   for(auto f = files_.cbegin(); f != files_.cend(); ++f)
   {
      (*f)->Shrink();
   }
}

//------------------------------------------------------------------------------

void Library::Shutdown(RestartLevel level)
{
   Debug::ft("Library.Shutdown");

   Restart::Release(sourcePathCfg_);
}

//------------------------------------------------------------------------------

void Library::Startup(RestartLevel level)
{
   Debug::ft("Library.Startup");

   //  Recreate our configuration parameter if it no longer exists.
   //
   if(sourcePathCfg_ == nullptr)
   {
      FunctionGuard guard(Guard_MemUnprotect);
      sourcePathCfg_.reset(new CfgStrParm
         ("SourcePath", EMPTY_STR, "source code directory"));
      Singleton< CfgParmRegistry >::Instance()->BindParm(*sourcePathCfg_);
   }

   //  Create the library's sets if they don't exist.
   //
   if(varSet_ != nullptr) return;

   //  Create the fixed sets.
   //
   dirSet_ = new CodeDirSet(DirsStr, nullptr);
   fileSet_ = new CodeFileSet(FilesStr, nullptr);
   hdrSet_ = new CodeFileSet(HdrsStr, nullptr);
   cppSet_ = new CodeFileSet(CppsStr, nullptr);
   extSet_ = new CodeFileSet(ExtsStr, nullptr);
   subsSet_ = new CodeFileSet(SubsStr, nullptr);
   varSet_ = new LibraryVarSet(VarsStr);
}

//------------------------------------------------------------------------------

void Library::Trim(ostream& stream) const
{
   Debug::ft("Library.Trim");

   //  There was originally a >trim command that displayed
   //  files in build order, so retain this behavior.
   //
   auto order = fileSet_->SortInBuildOrder();

   for(auto f = order.cbegin(); f != order.cend(); ++f)
   {
      auto file = f->file;
      if(file->IsHeader()) file->Trim(&stream);
      ThisThread::Pause();
   }

   for(auto f = order.cbegin(); f != order.cend(); ++f)
   {
      auto file = f->file;
      if(file->IsCpp()) file->Trim(&stream);
      ThisThread::Pause();
   }
}
}
