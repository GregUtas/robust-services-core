//==============================================================================
//
//  SysFile.cpp
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
#include "SysFile.h"
#include <cstdio>
#include <fstream>
#include <ios>
#include "Debug.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name SysFile_CreateIstream = "SysFile.CreateIstream";

istreamPtr SysFile::CreateIstream(const char* streamName)
{
   Debug::ft(SysFile_CreateIstream);

   auto stream = istreamPtr(new std::ifstream(streamName));

   if((stream != nullptr) && (stream->peek() == EOF))
   {
      stream.reset();
      return nullptr;
   }

   return stream;
}

//------------------------------------------------------------------------------

fn_name SysFile_CreateOstream = "SysFile.CreateOstream";

ostreamPtr SysFile::CreateOstream(const char* streamName, bool trunc)
{
   Debug::ft(SysFile_CreateOstream);

   auto mode = (trunc ? std::ios::trunc : std::ios::app);
   auto stream = ostreamPtr(new std::ofstream(streamName, mode));
   if(stream != nullptr) *stream << std::boolalpha << std::nouppercase;
   return stream;
}
}
