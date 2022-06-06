//==============================================================================
//
//  Element.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "MainArgs.h"
#include "Singleton.h"
#include "SystemTime.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string DefaultElementName = "Unnamed Element";

//------------------------------------------------------------------------------

Element::Element()
{
   Debug::ft("Element.ctor");

   //  Create our configuration parameters.
   //
   auto reg = Singleton<CfgParmRegistry>::Instance();

   nameCfg_.reset(new CfgStrParm
      ("ElementName", DefaultElementName, "element's name"));
   reg->BindParm(*nameCfg_);

   runningInLabCfg_.reset(new CfgBoolParm
      ("RunningInLab", "T", "set if running in lab"));
   reg->BindParm(*runningInLabCfg_);
}

//------------------------------------------------------------------------------

fn_name Element_dtor = "Element.dtor";

Element::~Element()
{
   Debug::ftnt(Element_dtor);

   Debug::SwLog(Element_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

const string Element::ConsoleFileName()
{
   return "console" + to_string(SystemTime::TimeZero(), FullNumeric) + ".txt";
}

//------------------------------------------------------------------------------

void Element::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "name            : " << Name() << CRLF;
   stream << prefix << "RunningInLab    : " << RunningInLab() << CRLF;
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

bool Element::IsUnnamed()
{
   Debug::ft("Element.IsUnnamed");

   return (Name() == DefaultElementName);
}

//------------------------------------------------------------------------------

string Element::Name()
{
   auto element = Singleton<Element>::Extant();
   if(element == nullptr) return DefaultElementName;
   return element->nameCfg_->CurrValue();
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
   //  Return the last directory named "rsc" on the path to the executable.
   //
   string path(MainArgs::At(0));
   string dir("rsc");
   dir.push_back(PATH_SEPARATOR);
   auto pos = path.rfind(dir);

   if(pos != string::npos)
   {
      path.erase(pos + 3);
   }
   else
   {
      //  An "rsc" directory was not found.  Set RscDir to the executable's
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
   //  This might be invoked very early during initialization, so don't
   //  create this class until it is required for another purpose.
   //
   auto element = Singleton<Element>::Extant();
   if(element != nullptr) return element->runningInLabCfg_->CurrValue();

#ifndef FIELD_LOAD
   return true;
#else
   return false;
#endif
}

//------------------------------------------------------------------------------

string Element::strTimePlace()
{
   return to_string(SystemTime::Now(), FullAlpha) + " on " + Name();
}
}
