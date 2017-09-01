//==============================================================================
//
//  CfgParm.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CfgParm.h"
#include <sstream>
#include "Algorithms.h"
#include "CfgParmRegistry.h"
#include "CfgTuple.h"
#include "Debug.h"
#include "Formatters.h"
#include "Log.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CfgParm_ctor = "CfgParm.ctor";

CfgParm::CfgParm(const char* key, const char* def, const char* expl) :
   tuple_(nullptr),
   default_(def),
   expl_(expl),
   level_(RestartNil)
{
   Debug::ft(CfgParm_ctor);

   Debug::Assert(key != nullptr);
   Debug::Assert(default_ != nullptr);
   Debug::Assert(expl_ != nullptr);

   auto reg = Singleton< CfgParmRegistry >::Instance();

   tuple_ = reg->FindTuple(key);

   if(tuple_ == nullptr)
   {
      tuple_ = new CfgTuple(key, default_);
      reg->BindTuple(*tuple_);
   }
}

//------------------------------------------------------------------------------

fn_name CfgParm_dtor = "CfgParm.dtor";

CfgParm::~CfgParm()
{
   Debug::ft(CfgParm_dtor);

   Singleton< CfgParmRegistry >::Instance()->UnbindParm(*this);
}

//------------------------------------------------------------------------------

void CfgParm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "tuple : ";

   if(tuple_ != nullptr)
   {
      stream << CRLF;
      tuple_->Display(stream, prefix + spaces(2), options);
   }
   else
   {
      stream << "undefined" << CRLF;
   }

   stream << prefix << "default : " << default_ << CRLF;
   stream << prefix << "expl    : " << expl_ << CRLF;
   stream << prefix << "level   : " << level_ << CRLF;
   stream << prefix << "link    : " << link_.to_str() << CRLF;
}

//------------------------------------------------------------------------------

fn_name CfgParm_GetCurr = "CfgParm.GetCurr";

string CfgParm::GetCurr() const
{
   Debug::ft(CfgParm_GetCurr);

   //  This is a pure virtual function.
   //
   Debug::SwErr(CfgParm_GetCurr, Key(), 0);
   return EMPTY_STR;
}

//------------------------------------------------------------------------------

fn_name CfgParm_GetInput = "CfgParm.GetInput";

string CfgParm::GetInput() const
{
   Debug::ft(CfgParm_GetInput);

   return GetCurr();
}

//------------------------------------------------------------------------------

const char* CfgParm::Key() const
{
   return tuple_->Key().c_str();
}

//------------------------------------------------------------------------------

ptrdiff_t CfgParm::LinkDiff()
{
   int local;
   auto fake = reinterpret_cast< const CfgParm* >(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

void CfgParm::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CfgParm_SetCurr = "CfgParm.SetCurr";

void CfgParm::SetCurr()
{
   Debug::ft(CfgParm_SetCurr);

   auto input = GetInput();
   tuple_->SetInput(input);
   level_ = RestartNil;
}

//------------------------------------------------------------------------------

fn_name CfgParm_SetFromTuple = "CfgParm.SetFromTuple";

bool CfgParm::SetFromTuple()
{
   Debug::ft(CfgParm_SetFromTuple);

   auto input = tuple_->Input();

   if(SetNext(input))
   {
      SetCurr();
      return true;
   }

   auto log = Log::Create("CFGPARM VALUE INVALID");

   if(log != nullptr)
   {
      *log << "errval=" << input << " key=" << Key() << CRLF;
      Log::Spool(log);
   }

   SetNext(default_);
   SetCurr();
   return false;
}

//------------------------------------------------------------------------------

fn_name CfgParm_SetNext = "CfgParm.SetNext";

bool CfgParm::SetNext(const std::string& input)
{
   Debug::ft(CfgParm_SetNext);

   //  This is a pure virtual function.
   //
   Debug::SwErr(CfgParm_SetNext, Key(), 0);
   return false;
}

//------------------------------------------------------------------------------

fn_name CfgParm_SetValue = "CfgParm.SetValue";

bool CfgParm::SetValue(const string& input, RestartLevel& level)
{
   Debug::ft(CfgParm_SetValue);

   if(!SetNext(input)) return false;
   level = RestartRequired();
   if(level == RestartNil) SetCurr();
   return true;
}
}
