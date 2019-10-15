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
#include <vector>
#include "CfgBoolParm.h"
#include "CfgParmRegistry.h"
#include "CfgStrParm.h"
#include "Clock.h"
#include "Debug.h"
#include "Formatters.h"
#include "Singleton.h"
#include "SysFile.h"
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

const string& Element::ConsoleFileName()
{
   static string ConsoleTranscriptFile;

   //  Set RscDir to the last directory named "rsc/" on the path to the
   //  executable.
   //
   if(ConsoleTranscriptFile.empty())
      ConsoleTranscriptFile = "console" + Clock::TimeZeroStr();
   return ConsoleTranscriptFile;
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
   stream << prefix << "ConsoleFileName : " << ConsoleFileName() << CRLF;
   stream << prefix << "RunningInLab : " << RunningInLab_ << CRLF;
   stream << prefix << "name         : " << strObj(name_.get()) << CRLF;
   stream << prefix << "runningInLab : " << strObj(runningInLab_.get()) << CRLF;
}

//------------------------------------------------------------------------------

const string& Element::HelpPath()
{
   static string HelpDir;

   if(HelpDir.empty())
   {
      auto rsc = RscPath();
      if(!rsc.empty()) HelpDir = rsc + PATH_SEPARATOR + "help";
   }

   return HelpDir;
}

//------------------------------------------------------------------------------

const string& Element::InputPath()
{
   static string InputDir;

   if(InputDir.empty())
   {
      auto rsc = RscPath();
      if(!rsc.empty()) InputDir = rsc + PATH_SEPARATOR + "input";
   }

   return InputDir;
}

//------------------------------------------------------------------------------

const string& Element::OutputPath()
{
   static string OutputDir;

   if(OutputDir.empty())
   {
      auto rsc = RscPath();
      if(!rsc.empty()) OutputDir = rsc + PATH_SEPARATOR + "excluded/output";
   }

   return OutputDir;
}

//------------------------------------------------------------------------------

void Element::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

const string& Element::RscPath()
{
   static string RscDir;

   //  Set RscDir to the last directory named "rsc/" on the path to the
   //  executable.
   //
   if(RscDir.empty())
   {
      auto reg = Singleton< CfgParmRegistry >::Extant();
      if(reg == nullptr) return RscDir;
      auto& args = reg->GetMainArgs();
      if(args.empty()) return RscDir;
      RscDir = *args.at(0);
      SysFile::Normalize(RscDir);

      string dir("rsc");
      dir.push_back(PATH_SEPARATOR);
      auto pos = RscDir.rfind(dir);

      if(pos != string::npos)
      {
         RscDir.erase(pos + 3);
      }
      else
      {
         //  An "rsc/" directory was not found.  Set RscDir to the executable's
         //  directory, though this is unlikely to work.
         //
         pos = RscDir.rfind(PATH_SEPARATOR);
         RscDir.erase(pos);
      }
   }

   return RscDir;
}

//------------------------------------------------------------------------------

string Element::strTimePlace()
{
   return (SysTime().to_str(SysTime::Alpha) + " on " + Name());
}
}
