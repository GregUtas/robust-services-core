//==============================================================================
//
//  CliBoolParm.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CliBoolParm.h"
#include <cctype>
#include <string>
#include "CliBuffer.h"
#include "CliCookie.h"
#include "CliThread.h"
#include "Debug.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string CliBoolParm::AnyBoolParm = "t|f";

//------------------------------------------------------------------------------

fn_name CliBoolParm_ctor = "CliBoolParm.ctor";

CliBoolParm::CliBoolParm(const char* help, bool opt, const char* tag) :
   CliParm(help, opt, tag)
{
   Debug::ft(CliBoolParm_ctor);
}

//------------------------------------------------------------------------------

fn_name CliBoolParm_dtor = "CliBoolParm.dtor";

CliBoolParm::~CliBoolParm()
{
   Debug::ft(CliBoolParm_dtor);
}

//------------------------------------------------------------------------------

fn_name CliBoolParm_GetBoolParmRc = "CliBoolParm.GetBoolParmRc";

CliParm::Rc CliBoolParm::GetBoolParmRc(bool& b, CliThread& cli) const
{
   Debug::ft(CliBoolParm_GetBoolParmRc);

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

   if(rc == Ok)
   {
      //  A string was found.  See if it is a boolean.
      //
      if(s.size() == 1)
      {
         auto c = tolower(s.front());

         if(c == 't')
         {
            b = true;
            cli.Cookie().Advance();
            return Ok;
         }

         if(c == 'f')
         {
            b = false;
            cli.Cookie().Advance();
            return Ok;
         }
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

   //  Error.  Highlight the location where a boolean was expected.
   //
   cli.ibuf->ErrorAtPos(cli, "Boolean expected", x);
   cli.Cookie().Advance();
   return Error;
}

//------------------------------------------------------------------------------

void CliBoolParm::Patch(sel_t selector, void* arguments)
{
   CliParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CliBoolParm_ShowValues = "CliBoolParm.ShowValues";

bool CliBoolParm::ShowValues(string& values) const
{
   Debug::ft(CliBoolParm_ShowValues);

   values = AnyBoolParm;
   return true;
}
}
