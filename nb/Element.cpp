//==============================================================================
//
//  Element.cpp
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
#include "Element.h"
#include <ostream>
#include "CfgBoolParm.h"
#include "CfgParmRegistry.h"
#include "CfgStrParm.h"
#include "Debug.h"
#include "Formatters.h"
#include "Singleton.h"
#include "SysTime.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
string Element::Name_ = "Unnamed Element";

#ifdef FIELD_LOAD
   bool Element::RunningInLab_ = false;
#else
   bool Element::RunningInLab_ = true;
#endif

//------------------------------------------------------------------------------

fn_name Element_ctor = "Element.ctor";

Element::Element() :
   name_(nullptr),
   runningInLab_(nullptr)
{
   Debug::ft(Element_ctor);

   //  Create our configuration parameters.
   //
   auto reg = Singleton< CfgParmRegistry >::Instance();

   name_.reset(new CfgStrParm
      ("ElementName", "Unnamed Element", &Name_, "element's name"));
   reg->BindParm(*name_);

   runningInLab_.reset(new CfgBoolParm
      ("RunningInLab", "T", &RunningInLab_, "set if running in lab"));
   reg->BindParm(*runningInLab_);
}

//------------------------------------------------------------------------------

fn_name Element_dtor = "Element.dtor";

Element::~Element()
{
   Debug::ft(Element_dtor);
}

//------------------------------------------------------------------------------

void Element::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "Name         : " << Name_ << CRLF;
   stream << prefix << "RscPath      : " << RscPath() << CRLF;
   stream << prefix << "HelpPath     : " << HelpPath() << CRLF;
   stream << prefix << "InputPath    : " << InputPath() << CRLF;
   stream << prefix << "OutputPath   : " << OutputPath() << CRLF;
   stream << prefix << "RunningInLab : " << RunningInLab_ << CRLF;
   stream << prefix << "name         : " << strObj(name_.get()) << CRLF;
   stream << prefix << "runningInLab : " << strObj(runningInLab_.get()) << CRLF;
}

//------------------------------------------------------------------------------

string Element::HelpPath()
{
   static string HelpDir;

   if(HelpDir.empty()) HelpDir = RscPath() + PATH_SEPARATOR + "help";
   return HelpDir;
}

//------------------------------------------------------------------------------

string Element::InputPath()
{
   static string InputDir;

   if(InputDir.empty()) InputDir = RscPath() + PATH_SEPARATOR + "input";
   return InputDir;
}

//------------------------------------------------------------------------------

string Element::OutputPath()
{
   static string OutputDir;

   if(OutputDir.empty())
      OutputDir = RscPath() + PATH_SEPARATOR + "excluded/output";
   return OutputDir;
}

//------------------------------------------------------------------------------

void Element::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

string Element::RscPath()
{
   static string RscDir;

   if(RscDir.empty())
   {
      auto& args = Singleton< CfgParmRegistry >::Instance()->GetMainArgs();
      RscDir = *args.at(0);

      for(size_t pos = 0; pos < RscDir.size(); ++pos)
      {
         if(RscDir[pos] == BACKSLASH) RscDir[pos] = PATH_SEPARATOR;
      }

      string dir("rsc");
      dir.push_back(PATH_SEPARATOR);
      auto pos = RscDir.rfind(dir);
      if(pos == string::npos) abort();
      RscDir.erase(pos + 3);
   }

   return RscDir;
}

//------------------------------------------------------------------------------

string Element::strTimePlace()
{
   return (SysTime().to_str(SysTime::Alpha) + " on " + Name());
}
}
