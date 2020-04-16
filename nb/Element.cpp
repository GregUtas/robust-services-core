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
fn_name Element_ctor = "Element.ctor";

Element::Element() :
   name_("Unnamed Element"),
   runningInLab_(nullptr)
{
   Debug::ft(Element_ctor);

   runningInLab_ = new bool;

#ifdef FIELD_LOAD
   *runningInLab_ = false;
#else
   *runningInLab_ = true;
#endif

   //  Create our configuration parameters.
   //
   auto reg = Singleton< CfgParmRegistry >::Instance();

   nameCfg_.reset(new CfgStrParm
      ("ElementName", "Unnamed Element", &name_, "element's name"));
   reg->BindParm(*nameCfg_);

   runningInLabCfg_.reset(new CfgBoolParm
      ("RunningInLab", "T", runningInLab_, "set if running in lab"));
   reg->BindParm(*runningInLabCfg_);
}

//------------------------------------------------------------------------------

fn_name Element_dtor = "Element.dtor";

Element::~Element()
{
   Debug::ft(Element_dtor);

   delete runningInLab_;
   runningInLab_ = nullptr;
}

//------------------------------------------------------------------------------

const string Element::ConsoleFileName()
{
   return "console" + Clock::TimeZeroStr();
}

//------------------------------------------------------------------------------

void Element::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "name            : " << name_ << CRLF;
   stream << prefix << "runningInLab    : " << *runningInLab_ << CRLF;
   stream << prefix << "RscPath         : " << RscPath() << CRLF;
   stream << prefix << "HelpPath        : " << HelpPath() << CRLF;
   stream << prefix << "InputPath       : " << InputPath() << CRLF;
   stream << prefix << "OutputPath      : " << OutputPath() << CRLF;
   stream << prefix << "ConsoleFileName : " << ConsoleFileName() << CRLF;
   stream << prefix << "nameCfg         : " << strObj(nameCfg_.get()) << CRLF;
   stream << prefix << "runningInLabCfg : "
      << strObj(runningInLabCfg_.get()) << CRLF;
}

//------------------------------------------------------------------------------

const string Element::HelpPath()
{
   auto path = RscPath();

   if(!path.empty())
   {
      path.push_back(PATH_SEPARATOR);
      path.append("help");
   }

   return path;
}

//------------------------------------------------------------------------------

const string Element::InputPath()
{
   auto path = RscPath();

   if(!path.empty())
   {
      path.push_back(PATH_SEPARATOR);
      path.append("input");
   }

   return path;
}

//------------------------------------------------------------------------------

string Element::Name()
{
   return Singleton< Element >::Instance()->name_.c_str();
}

//------------------------------------------------------------------------------

const string Element::OutputPath()
{
   auto path = RscPath();

   if(!path.empty())
   {
      path.push_back(PATH_SEPARATOR);
      path.append("excluded/output");
   }

   return path;
}

//------------------------------------------------------------------------------

void Element::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

const string Element::RscPath()
{
   //  Return the last directory named "rsc/" on the path to the executable.
   //
   auto reg = Singleton< CfgParmRegistry >::Extant();
   if(reg == nullptr) return EMPTY_STR;

   auto& args = reg->GetMainArgs();
   if(args.empty()) return EMPTY_STR;

   string path(args.at(0)->c_str());
   SysFile::Normalize(path);

   string dir("rsc");
   dir.push_back(PATH_SEPARATOR);
   auto pos = path.rfind(dir);

   if(pos != string::npos)
   {
      path.erase(pos + 3);
   }
   else
   {
      //  An "rsc/" directory was not found.  Set RscDir to the executable's
      //  directory, though this is unlikely to work.
      //
      pos = path.rfind(PATH_SEPARATOR);
      path.erase(pos);
   }

   return path;
}

//------------------------------------------------------------------------------

bool Element::RunningInLab()
{
   return *Singleton< Element >::Instance()->runningInLab_;
}

//------------------------------------------------------------------------------

string Element::strTimePlace()
{
   return (SysTime().to_str(SysTime::Alpha) + " on " + Name());
}
}
