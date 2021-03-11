//==============================================================================
//
//  CxxLocation.h
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
#ifndef CXXLOCATION_H_INCLUDED
#define CXXLOCATION_H_INCLUDED

#include <cstddef>
#include <string>
#include "CodeTypes.h"
#include "LibraryTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  Where a C++ item was declared or defined.
//
class CxxLocation
{
public:
   //  Initializes fields to default values.
   //
   CxxLocation();

   //  Copy constructor.
   //
   CxxLocation(const CxxLocation& that) = default;

   //  Copy operator.
   //
   CxxLocation& operator=(const CxxLocation& that) = default;

   //  Records the item's location in source code.
   //
   void SetLoc(CodeFile* file, size_t pos);

   //  Returns the file in which the item is located.  A template instance
   //  belongs to the file that caused its instantiation.  An item added by
   //  the Editor belongs to the file to which it was added.
   //
   CodeFile* GetFile() const { return file_; }

   //  Returns the start of the item's position within its file, which is an
   //  index into a string that contains the file's contents.  For a template
   //  instance, this is an offset into its internally generated code.  For
   //  an item added by the Editor, string::npos is returned.
   //
   size_t GetPos() const { return (erased_ ? std::string::npos : pos_); }

   //  Updates the item's location after code has been edited.  Has the same
   //  interface as CxxToken::UpdatePos.
   //
   void UpdatePos(EditorAction action, size_t begin, size_t count, size_t from);

   //  Marks the item as internally generated.
   //
   void SetInternal() { internal_ = true; }

   //  Returns true for an internally generated item, such as the code
   //  for a template instance.
   //
   bool IsInternal() const { return internal_; }
private:
   //  The file in which the item appeared.
   //
   CodeFile* file_;

   //  The item's location in FILE.  The file has a string member that holds
   //  the code, and this is an index into that string.
   //
   size_t pos_;

   //  Set if the item has been erased during editing.
   //
   bool erased_;

   //  Set if the item appeared in internally generated code, which currently
   //  means in a template instance.
   //
   bool internal_;
};
}
#endif
