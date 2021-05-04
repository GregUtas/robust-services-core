//==============================================================================
//
//  CxxLocation.cpp
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
#include "CxxLocation.h"
#include "CxxExecute.h"

using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
CxxLocation::CxxLocation() :
   file_(nullptr),
   pos_(string::npos),
   erased_(false),
   internal_(!Context::ParsingSourceCode())
{
}

//------------------------------------------------------------------------------

void CxxLocation::SetLoc(CodeFile* file, size_t pos)
{
   file_ = file;
   pos_ = pos;
}

//------------------------------------------------------------------------------

void CxxLocation::SetLoc(CodeFile* file, size_t pos, bool internal)
{
   SetLoc(file, pos);
   internal_ = internal;
}

//------------------------------------------------------------------------------

void CxxLocation::UpdatePos(EditorAction action,
   size_t begin, size_t count, size_t from)
{
   if(pos_ == string::npos) return;
   if(internal_) return;

   switch(action)
   {
   case Erased:
      if(!erased_ && (pos_ >= begin))
      {
         if(pos_ < (begin + count))
            erased_ = true;
         else
            pos_ -= count;
      }
      break;

   case Inserted:
      if(!erased_ && (pos_ >= begin)) pos_ += count;
      break;

   case Pasted:
      if(erased_)
      {
         if((pos_ >= from) && (pos_ < (from + count)))
         {
            pos_ = pos_ + begin - from;
            erased_ = false;
         }
      }
      else
      {
         if(pos_ >= begin) pos_ += count;
      }
      break;
   }
}
}
