//==============================================================================
//
//  CliText.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CliText.h"
#include <cstring>
#include <ostream>
#include <string>
#include "CliBuffer.h"
#include "CliCookie.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CliText_ctor = "CliText.ctor";

CliText::CliText(const char* help, const char* text, bool opt, size_t size) :
   CliParm(help, opt, nullptr),
   text_(text)
{
   Debug::ft(CliText_ctor);

   if(text_ == nullptr) text = EMPTY_STR;
   parms_.Init(size, CliParm::CellDiff(), MemProt);
}

//------------------------------------------------------------------------------

fn_name CliText_dtor = "CliText.dtor";

CliText::~CliText()
{
   Debug::ft(CliText_dtor);
}

//------------------------------------------------------------------------------

fn_name CliText_AccessParm = "CliText.AccessParm";

CliParm* CliText::AccessParm(CliCookie& cookie, size_t depth) const
{
   Debug::ft(CliText_AccessParm);

   //  We are currently at DEPTH in the parameter tree.  If we are looking
   //  for parameters at DEPTH + 1, go deeper into the tree to access the
   //  next parameter.
   //
   if(cookie.Index(depth + 1) > 0)
   {
      auto id = cookie.Index(depth);
      auto obj = parms_.At(id)->AccessParm(cookie, depth + 1);
      if(obj != nullptr) return obj;
      if(!Ascend()) return nullptr;
      cookie.Ascend();
   }

   //  If there is a parameter at DEPTH, return it, else return nullptr to
   //  cause backup to DEPTH - 1.
   //
   auto id = cookie.Index(depth);
   if(id <= Parms().Size()) return parms_.At(id);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name CliText_BindParm = "CliText.BindParm";

bool CliText::BindParm(CliParm& parm)
{
   Debug::ft(CliText_BindParm);

   //  Before adding PARM, ensure that its tag (if any) is unique.
   //
   auto tag = parm.Tag();

   if(tag != nullptr)
   {
      for(auto p = parms_.First(); p != nullptr; parms_.Next(p))
      {
         auto t = p->Tag();

         if((t != nullptr) && (strcmp(t, tag) == 0))
         {
            Debug::SwErr(CliText_BindParm, Parms().Size(), 1);
            return false;
         }
      }
   }

   return parms_.Insert(parm);
}

//------------------------------------------------------------------------------

fn_name CliText_Descend = "CliText.Descend";

void CliText::Descend(CliCookie& cookie) const
{
   Debug::ft(CliText_Descend);

   //  If the string that was just read takes no parameters, advance to
   //  the next parameter at this level, else descend one level to look
   //  for the string's parameters.
   //
   if(parms_.Empty())
      cookie.Advance();
   else
      cookie.Descend();
}

//------------------------------------------------------------------------------

void CliText::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   CliParm::Display(stream, prefix, options);

   stream << prefix << "text  : " << text_ << CRLF;
   stream << prefix << "parms : " << CRLF;
   parms_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name CliText_Explain = "CliText.Explain";

void CliText::Explain(ostream& stream, col_t indent) const
{
   Debug::ft(CliText_Explain);

   //  Use our base class to display our string, and follow it with our
   //  parameters.
   //
   CliParm::Explain(stream, indent);

   for(auto p = parms_.First(); p != nullptr; parms_.Next(p))
   {
      p->Explain(stream, indent + 2);
   }
}

//------------------------------------------------------------------------------

fn_name CliText_GetTextParmRc = "CliText.GetTextParmRc";

CliParm::Rc CliText::GetTextParmRc(id_t& i, string& s, CliThread& cli) const
{
   Debug::ft(CliText_GetTextParmRc);

   i = 0;

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
         s.clear();
         cli.ibuf->SetPos(x);
         cli.Cookie().Advance();
         return None;
      }
   }

   if(rc == Ok)
   {
      //  A string was found.  See if it matches this parameter's text_
      //  string.  An empty text_ string accepts all string inputs.
      //
      if((strlen(text_) == 0) || (s.compare(text_) == 0))
      {
         i = 1;
         Descend(cli.Cookie());
         return Ok;
      }
   }

   //  A valid parameter was not found.  This is an error unless the
   //  parameter is optional and was untagged, in which case we report
   //  its absence after backing up if the skip character was entered.
   //
   if(IsOptional() && !tagged)
   {
      s.clear();
      if(rc != Skip) cli.ibuf->SetPos(x);
      cli.Cookie().Advance();
      return None;
   }

   //  Error.  Highlight the location where a string was expected.
   //
   s.clear();
   cli.ibuf->ErrorAtPos(cli, "Specific string value expected", x);
   cli.Cookie().Advance();
   return Error;
}

//------------------------------------------------------------------------------

const char* CliText::HelpText() const
{
   if(strlen(text_) > 0) return text_;
   return AnyStringParm;
}

//------------------------------------------------------------------------------

void CliText::Patch(sel_t selector, void* arguments)
{
   CliParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CliText_ShowValues = "CliText.ShowValues";

bool CliText::ShowValues(string& values) const
{
   Debug::ft(CliText_ShowValues);

   values = HelpText();
   return false;
}
}
