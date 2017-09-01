//==============================================================================
//
//  FunctionStats.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "FunctionStats.h"
#include <iomanip>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Formatters.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeTools
{
FunctionStats::FunctionStats(fn_name_arg func, size_t calls) :
   func_(func),
   calls_(calls),
   time_(0)
{
}

//------------------------------------------------------------------------------

FunctionStats::~FunctionStats() { }

//------------------------------------------------------------------------------

void FunctionStats::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   stream << setw(9) << calls_ << spaces(2);
   stream << setw(10) << time_ << spaces(3);
   stream << func_ << CRLF;
}

//------------------------------------------------------------------------------

void FunctionStats::IncrCalls(usecs_t net)
{
   ++calls_;
   time_ += net;
}

//------------------------------------------------------------------------------

ptrdiff_t FunctionStats::LinkDiff()
{
   int local;
   auto fake = reinterpret_cast< const FunctionStats* >(&local);
   return ptrdiff(&fake->link_, fake);
}
}
