//==============================================================================
//
//  Log.h
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
#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#include <string>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Interface for generating logs.
//
class Log
{
public:
   //  Creates a log in which the first line will be TITLE, followed
   //  by Element::strTimePlace().
   //
   static ostringstreamPtr Create(fixed_string title);

   //  Spools LOG to the log thread.  The log system assumes ownership of
   //  LOG, which is set to nullptr.
   //
   static void Spool(ostringstreamPtr& log);

   //  Returns the name of the log file.
   //
   static std::string FileName();
private:
   //  Deleted because this class only has static members.
   //
   Log() = delete;
};
}
#endif
