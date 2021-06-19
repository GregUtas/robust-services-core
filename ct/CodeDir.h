//==============================================================================
//
//  CodeDir.h
//
//  Copyright (C) 2013-2021  Greg Utas
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
#ifndef CODEDIR_H_INCLUDED
#define CODEDIR_H_INCLUDED

#include "LibraryItem.h"
#include <cstddef>
#include <string>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Provides access to a directory that contains source code.
//
class CodeDir : public LibraryItem
{
public:
   //  Creates an instance for PATH, which will be referred to by NAME.
   //
   CodeDir(const std::string& name, const std::string& path);

   //  Not subclassed.
   //
   ~CodeDir();

   //  Finds all of the .h and .cpp files in the directory.  Updates
   //  EXPL to indicate success or failure.  Returns 0 on success.
   //  The implementation is platform specific because C++ does not
   //  provide a way to iterate over the files in a directory.
   //
   NodeBase::word Extract(std::string& expl);

   //  Returns the directory's path.
   //
   const std::string& Path() const { return path_; }

   //  Returns true if the directory contains substitute files.
   //
   bool IsSubsDir() const;

   //  Returns the number of .h files in the directory.
   //
   size_t HeaderCount() const;

   //  Returns the number of .cpp files in the directory.
   //
   size_t CppCount() const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to update ITEMS with ones declared by the directory's files.
   //
   void GetDecls(CxxNamedSet& items) override;

   //  Overridden to return the item's name.
   //
   const std::string& Name() const override { return name_; }
private:
   //  The set's name.
   //
   const std::string name_;

   //  The directory's path.
   //
   const std::string path_;
};
}
#endif
