//==============================================================================
//
//  CfgTuple.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CfgTuple.h"
#include <sstream>
#include "Algorithms.h"
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "Log.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const char CfgTuple::CommentChar = '/';

//------------------------------------------------------------------------------

fn_name CfgTuple_ctor = "CfgTuple.ctor";

CfgTuple::CfgTuple(const string& key, const string& input) :
   key_(key),
   input_(input)
{
   Debug::ft(CfgTuple_ctor);

   if(key_.find_first_not_of(ValidNameChars()) != string::npos)
   {
      auto log = Log::Create("CFGPARM INVALID KEY");

      if(log != nullptr)
      {
         *log << "errval=" << key_ << CRLF;
         Log::Spool(log);
      }
   }
}

//------------------------------------------------------------------------------

fn_name CfgTuple_dtor = "CfgTuple.dtor";

CfgTuple::~CfgTuple()
{
   Debug::ft(CfgTuple_dtor);

   Singleton< CfgParmRegistry >::Instance()->UnbindTuple(*this);
}

//------------------------------------------------------------------------------

void CfgTuple::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "key   : " << key_ << CRLF;
   stream << prefix << "input : " << input_ << CRLF;
   stream << prefix << "link  : " << link_.to_str() << CRLF;
}

//------------------------------------------------------------------------------

ptrdiff_t CfgTuple::LinkDiff()
{
   int local;
   auto fake = reinterpret_cast< const CfgTuple* >(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

void CfgTuple::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

const string& CfgTuple::ValidBlankChars()
{
   //> Valid blank characters in a file that contains configuration tuples.
   //
   static const string BlankChars(" ");

   return BlankChars;
}

//------------------------------------------------------------------------------

const string& CfgTuple::ValidNameChars()
{
   //> Valid characters in a configuration tuple's name.
   //
   static const string NameChars
      ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-.");

   return NameChars;
}

//------------------------------------------------------------------------------

const string& CfgTuple::ValidValueChars()
{
   //> Valid characters in a configuration tuple's value.
   //
   static const string ValueChars(ValidNameChars() + ":/\\");

   return ValueChars;
}
}
