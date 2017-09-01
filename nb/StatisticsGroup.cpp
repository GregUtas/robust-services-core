//==============================================================================
//
//  StatisticsGroup.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "StatisticsGroup.h"
#include <ostream>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "Singleton.h"
#include "StatisticsRegistry.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const size_t StatisticsGroup::MaxExplSize = 44;
const size_t StatisticsGroup::ReportWidth = 76;
fixed_string StatisticsGroup::ReportHeader =   "      Curr      Prev         All";
//  <----------------group name---------------->      Curr      Prev         All
//    <-------------member name---------------->
//      <---individual statistic explanation---> nnnnnnnnn nnnnnnnnn nnnnnnnnnnn
// 0        1         2         3         4         5         6         7
// 01234567890123456789012345678901234567890123456789012345678901234567890123456

//------------------------------------------------------------------------------

fn_name StatisticsGroup_ctor = "StatisticsGroup.ctor";

StatisticsGroup::StatisticsGroup(const string& expl) : expl_(expl.c_str())
{
   Debug::ft(StatisticsGroup_ctor);

   if(expl_.size() > MaxExplSize)
   {
      Debug::SwErr(StatisticsGroup_ctor, expl_.size(), 0);
   }

   Singleton< StatisticsRegistry >::Instance()->BindGroup(*this);
}

//------------------------------------------------------------------------------

fn_name StatisticsGroup_dtor = "StatisticsGroup.dtor";

StatisticsGroup::~StatisticsGroup()
{
   Debug::ft(StatisticsGroup_dtor);

   if(Gid() != NIL_ID)
   {
      Singleton< StatisticsRegistry >::Instance()->UnbindGroup(*this);
   }
}

//------------------------------------------------------------------------------

ptrdiff_t StatisticsGroup::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const StatisticsGroup* >(&local);
   return ptrdiff(&fake->gid_, fake);
}

//------------------------------------------------------------------------------

void StatisticsGroup::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "gid  : " << gid_.to_str() << CRLF;
   stream << prefix << "expl : " << expl_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name StatisticsGroup_DisplayStats = "StatisticsGroup.DisplayStats";

void StatisticsGroup::DisplayStats(ostream& stream, id_t id) const
{
   Debug::ft(StatisticsGroup_DisplayStats);

   stream << expl_;
   stream << spaces(MaxExplSize - expl_.size()) << ReportHeader << CRLF;
}

//------------------------------------------------------------------------------

void StatisticsGroup::Patch(sel_t selector, void* arguments)
{
   Dynamic::Patch(selector, arguments);
}
}
