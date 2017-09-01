//==============================================================================
//
//  SysFile.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
