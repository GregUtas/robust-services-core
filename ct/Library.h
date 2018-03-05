//==============================================================================
//
//  Library.h
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
#ifndef LIBRARY_H_INCLUDED
#define LIBRARY_H_INCLUDED

#include "Temporary.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include "NbTypes.h"
#include "Q2Way.h"
#include "Registry.h"
#include "SysTypes.h"

namespace CodeTools
{
   class CodeDir;
   class CodeDirSet;
   class CodeFile;
   class CodeFileSet;
   class LibrarySet;
   class LibraryVarSet;
}

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Provides access to the source code being analyzed by tools.
//
class Library : public NodeBase::Temporary
{
   friend class NodeBase::Singleton< Library >;
public:
   //  Returns the path to the source directory, the root for all .h and
   //  .cpp files.  Does not include a trailing PATH_SEPARATOR character.
   //
   static const std::string& SourcePath() { return SourcePath_; }

   //  Adds PATH, which will be known by NAME, to the code base.  Updates
   //  EXPL to indicate success or failure.  Returns 0 on success.
   //
   NodeBase::word Import
      (const std::string& name, const std::string& path, std::string& expl);

   //  Returns the directory identified by NAME.
   //
   CodeDir* FindDir(const std::string& name) const;

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

   //  Returns the variable identified by NAME.
   //
   LibrarySet* FindVar(const std::string& name) const;

   //  If S is a variable, it is returned.  If S is the name of a directory
   //  or file, a single-member temporary set for it is created and returned.
   //  On failure, nullptr is returned.
   //
   LibrarySet* EnsureVar(const std::string& s) const;

   //  Removes VAR from the list of variables.
   //
   void EraseVar(LibrarySet& var);

   //  Assigns the results of EXPR to NAME.  POS is where EXPR started in the
   //  input stream.  Updates EXPL to indicate success or failure.  Returns 0
   //  on success.
   //
   NodeBase::word Assign(const std::string& name,
      const std::string& expr, size_t pos, std::string& expl);

   //  Displays the library's contents in STREAM.  The characters in OPTS
   //  control formatting options.  Returns 0 on success.
   //
   NodeBase::word Export(std::ostream& stream, const std::string& opts) const;

   //  Deletes the variable known by NAME.  Updates EXPL to indicate success
   //  or failure.  Returns 0 on success.
   //
   NodeBase::word Purge(const std::string& name, std::string& expl) const;

   //  Returns the set associated with EXPR, which starts at offset POS of
   //  the input line.  The caller must invoke Release on the result after
   //  using it.
   //
   LibrarySet* Evaluate(const std::string& expr, size_t pos) const;

   //  Returns the registry of directories.  Used for iteration.
   //
   const NodeBase::Registry< CodeDir >& Directories() const { return dirs_; }

   //  Returns the registry of files.  Used for iteration.
   //
   const NodeBase::Registry< CodeFile >& Files() const { return files_; }

   //> The maximum number of directories supported.
   //
   static const size_t MaxDirs;

   //> The maximum number of files supported.
   //
   static const size_t MaxFiles;

   //> The name for the directory that contains substitute files (see
   //  the definition of subsSet_, below).
   //
   static NodeBase::fixed_string SubsDir;

   //  Returns the identifiers of files that declare external types.
   //
   const CodeFileSet* SubsFiles() const { return subsSet_; }

   //  Returns the queue of variables.  Used for iteration.
   //
   const NodeBase::Q2Way< LibrarySet >& Variables() const { return vars_; }

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for restarts.
   //
   virtual void Shutdown(NodeBase::RestartLevel level) override;

   //  Overridden for restarts.
   //
   virtual void Startup(NodeBase::RestartLevel level) override;
private:
   //  Private because this singleton is not subclassed.
   //
   Library();

   //  Private because this singleton is not subclassed.
   //
   ~Library();

   //  The path to the root directory for source code files.  This is
   //  deliberately static so that it survives restarts.
   //
   static std::string SourcePath_;

   //  Configuration parameter for the source code directory.
   //
   NodeBase::CfgStrParmPtr sourcePathCfg_;

   //  The directories in the code base.
   //
   NodeBase::Registry< CodeDir > dirs_;

   //  The files in the code base.
   //
   NodeBase::Registry< CodeFile > files_;

   //  The currently defined variables.
   //
   NodeBase::Q2Way< LibrarySet > vars_;

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
   //  files declare items that are external to the code base, and
   //  therefore acts as substitutes for the unparsed external files
   //  (extSet_).
   //
   CodeFileSet* subsSet_;

   //  A variable for the set of all variables.
   //
   LibraryVarSet* varSet_;
};
}
#endif
