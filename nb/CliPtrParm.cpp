//==============================================================================
//
//  CliPtrParm.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CliPtrParm.h"
#include <string>
#include "CliBuffer.h"
#include "CliCookie.h"
#include "CliIntParm.h"
#include "CliThread.h"
#include "Debug.h"
#include "SysTypes.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CliPtrParm_ctor = "CliPtrParm.ctor";

CliPtrParm::CliPtrParm(const char* help, bool opt, const char* tag) :
   CliParm(help, opt, tag)
{
   Debug::ft(CliPtrParm_ctor);
}

//------------------------------------------------------------------------------

fn_name CliPtrParm_dtor = "CliPtrParm.dtor";

CliPtrParm::~CliPtrParm()
{
   Debug::ft(CliPtrParm_dtor);
}

//------------------------------------------------------------------------------

fn_name CliPtrParm_GetPtrParmRc = "CliPtrParm.GetPtrParmRc";

CliParm::Rc CliPtrParm::GetPtrParmRc(void*& p, CliThread& cli) const
{
   Debug::ft(CliPtrParm_GetPtrParmRc);

   p = nullptr;

   string s;
   string t;

   //  Get the next string after saving the current location in the buffer.
   //
   auto x = cli.ibuf->Pos();
   auto rc = cli.ibuf->GetStr(t, s);
   auto tagged = (!t.empty());

   //  If a tag was found, then it must match this parameter's tag before
   //  we bother to look for the parameter itself.
   //
   if(tagged)
   {
      auto tag = Tag();

      if((tag == nullptr) || (t.compare(tag) != 0))
      {
         cli.ibuf->SetPos(x);
         cli.Cookie().Advance();
         return None;
      }
   }

   //  If the string is an integer within the pointer's range, return it.
   //
   if(rc == Ok)
   {
      word n;

      rc = CliBuffer::GetInt(s, n, true);

      if(rc == Ok)
      {
         p = (void*) n;
         cli.Cookie().Advance();
         return Ok;
      }
   }

   //  A valid parameter was not found.  This is an error unless the
   //  parameter is optional and was untagged, in which case we report
   //  its absence after backing up if the skip character was entered.
   //
   if(IsOptional() && !tagged)
   {
      if(rc != Skip) cli.ibuf->SetPos(x);
      cli.Cookie().Advance();
      return None;
   }

   cli.ibuf->ErrorAtPos(cli, "Pointer expected or out of range", x);
   cli.Cookie().Advance();
   return Error;
}

//------------------------------------------------------------------------------

void CliPtrParm::Patch(sel_t selector, void* arguments)
{
   CliParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CliPtrParm_ShowValues = "CliPtrParm.ShowValues";

bool CliPtrParm::ShowValues(string& values) const
{
   Debug::ft(CliPtrParm_ShowValues);

   values = CliIntParm::AnyHexParm;
   return false;
}
}
