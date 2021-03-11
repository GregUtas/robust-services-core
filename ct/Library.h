//==============================================================================
//
//  Library.h
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
#ifndef LIBRARY_H_INCLUDED
#define LIBRARY_H_INCLUDED

#include "Base.h"
#include <cstddef>
#include <iosfwd>
#include <list>
#include <string>
#include "CfgStrParm.h"
#include "LibraryTypes.h"
#include "NbTypes.h"
#include "SysTypes.h"

namespace NodeBase
{
   class CliThread;
}

namespace CodeTools
{
   class CodeDirSet;
   class CodeFileSet;
   class LibraryVarSet;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Provides access to the source code being analyzed by tools.
//
class Library : public NodeBase::Base
{
   friend class NodeBase::Singleton< Library >;
public:
   //  Returns the path to the source directory, the root for all .h and
   //  .cpp files.  Does not include a trailing PATH_SEPARATOR character.
   //
   NodeBase::c_string SourcePath() const {return sourcePathCfg_->GetValue(); }

   //  Adds PATH, which will be known by NAME, to the code base.  Updates
   //  EXPL to indicate success or failure.  Returns 0 on success.
   //
   NodeBase::word Import
      (const std::string& name, const std::string& path, std::string& expl);

   //  Returns FILE's entry in the code base.  If FILE does not have an
   //  entry, one is created.  DIR is FILE's directory, if known.
   //
   CodeFile* EnsureFile(const std::string& file, CodeDir* dir = nullptr);

   //  Adds FILE to the code base.
   //
   void AddFile(CodeFile& file);

   //  Returns the file identified by NAME.
   //
   CodeFile* FindFile(const std::string& name) const;

   //  Adds VAR to the list of variables.
   //
   void AddVar(LibrarySet& var);

   //  If S is a variable, it is returned.  If S is the name of a directory
   //  or file, a single-member temporary set for it is created and returned.
   //  On failure, nullptr is returned.
   //
   LibrarySet* EnsureVar(CliThread& cli, const std::string& s) const;

   //  Removes VAR from the list of variables.
   //
   void EraseVar(const LibrarySet* var);

   //  Assigns the results of EXPR to NAME.  POS is where EXPR started in the
   //  input stream.  Updates EXPL to indicate success or failure.  Returns 0
   //  on success.
   //
   NodeBase::word Assign(CliThread& cli, const std::string& name,
      const std::string& expr, size_t pos, std::string& expl);

   //  Displays the library's contents in STREAM.  The characters in OPTS
   //  control what information will be included.
   //
   void Export(std::ostream& stream, const std::string& opts) const;

   //  Displays each parsed file's symbol usage and recommended modifications
   //  to its #include directives, using statements, and forward declarations
   //  in STREAM.
   //
   void Trim(std::ostream& stream) const;

   //  Deletes the variable known by NAME.  Updates EXPL to indicate success
   //  or failure.  Returns 0 on success.
   //
   NodeBase::word Purge(const std::string& name, std::string& expl);

   //  Returns the set associated with EXPR, which starts at offset POS of
   //  the input line.  The caller must invoke Release on the result after
   //  using it.
   //
   LibrarySet* Evaluate
      (CliThread& cli, const std::string& expr, size_t pos) const;

   //  Returns all directories.  Used for iteration.
   //
   const CodeDirSet& Directories() const { return *dirSet_; }

   //  Returns all files.  Used for iteration.
   //
   const CodeFileSet& Files() const { return *fileSet_; }

   //  The name of the directory that contains substitute files (see
   //  the definition of subsSet_, below).
   //
   static NodeBase::fixed_string SubsDir;

   //  Returns all files that declare external types.
   //
   const CodeFileSet& SubsFiles() const { return *subsSet_; }

   //  Returns all variables.  Used for iteration.
   //
   const std::list< LibrarySet* > Variables() const { return vars_; }

   //  Shrinks containers.
   //
   void Shrink();

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for restarts.
   //
   void Shutdown(NodeBase::RestartLevel level) override;

   //  Overridden for restarts.
   //
   void Startup(NodeBase::RestartLevel level) override;
private:
   //  Private because this singleton is not subclassed.
   //
   Library();

   //  Private because this singleton is not subclassed.
   //
   ~Library();

   //  Returns the directory identified by NAME.
   //
   CodeDir* FindDir(const std::string& name) const;

   //  Returns the variable identified by NAME.
   //
   LibrarySet* FindVar(const std::string& name) const;

   //  Configuration parameter for the source code directory.
   //
   NodeBase::CfgStrParmPtr sourcePathCfg_;

   //  The directories in the code base.  Sorted by name, ignoring case.
   //
   std::list< std::unique_ptr < CodeDir >> dirs_;

   //  The files in the code base.  Sorted by name, ignoring case.
   //
   std::list< std::unique_ptr < CodeFile >> files_;

   //  The currently defined variables.  Sorted by name, ignoring case.
   //
   std::list< LibrarySet* > vars_;

   //  A variable for the set of all directories.
   //
   CodeDirSet* dirSet_;

   //  A variable for the set of all code files.
   //
   CodeFileSet* fileSet_;

   //  A variable for the set of all .h files.
   //
   CodeFileSet* hdrSet_;

   //  A variable for the set of all .cpp files.
   //
   CodeFileSet* cppSet_;

   //  A variable for the set of all external files.  An external
   //  file is a header that was #included but whose directory is
   //  not in dirs_ because it has yet to be defined using >import.
   //
   CodeFileSet* extSet_;

   //  A variable for the set of all substitute files.  Substitute
   //  files declare items that are external to the code base so
   //  that the full versions of those files (e.g. STL headers) do
   //  not have to be compiled.
   //
   CodeFileSet* subsSet_;

   //  A variable for the set of all variables.
   //
   LibraryVarSet* varSet_;
};
}
#endif
