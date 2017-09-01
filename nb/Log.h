//==============================================================================
//
//  Log.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
   //  Private because this class only has static members.
   //
   Log();
};
}
#endif
