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
#include <memory>
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
string Element::HelpPath_ = "help";
string Element::InputPath_ = "input";
string Element::OutputPath_ = "output";

#ifdef FIELD_LOAD
   bool Element::RunningInLab_ = false;
#else
   bool Element::RunningInLab_ = true;
#endif

//------------------------------------------------------------------------------

fn_name Element_ctor = "Element.ctor";

Element::Element() :
   name_(nullptr),
   inputPath_(nullptr),
   outputPath_(nullptr),
   runningInLab_(nullptr)
{
   Debug::ft(Element_ctor);

   //  Create our configuration parameters.
   //
   auto reg = Singleton< CfgParmRegistry >::Instance();

   name_.reset(new CfgStrParm
      ("ElementName", "Unnamed Element", &Name_, "element's name"));
   reg->BindParm(*name_);

   helpPath_.reset(new CfgStrParm
      ("HelpPath", "help", &HelpPath_, "help directory"));
   reg->BindParm(*helpPath_);

   inputPath_.reset(new CfgStrParm
      ("InputPath", "input", &InputPath_, "input directory"));
   reg->BindParm(*inputPath_);

   outputPath_.reset(new CfgStrParm
      ("OutputPath", "output", &OutputPath_, "output directory"));
   reg->BindParm(*outputPath_);

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
   stream << prefix << "InputPath    : " << HelpPath_ << CRLF;
   stream << prefix << "InputPath    : " << InputPath_ << CRLF;
   stream << prefix << "OutputPath   : " << OutputPath_ << CRLF;
   stream << prefix << "RunningInLab : " << RunningInLab_ << CRLF;
   stream << prefix << "name         : " << strObj(name_.get()) << CRLF;
   stream << prefix << "helpPath     : " << strObj(helpPath_.get()) << CRLF;
   stream << prefix << "inputPath    : " << strObj(inputPath_.get()) << CRLF;
   stream << prefix << "outputPath   : " << strObj(outputPath_.get()) << CRLF;
   stream << prefix << "runningInLab : " << strObj(runningInLab_.get()) << CRLF;
}

//------------------------------------------------------------------------------

const string& Element::HelpPath()
{
   //  A static default is not required for this string, because it is
   //  not used before the CLI has been entered.
   //
   return HelpPath_;
}

//------------------------------------------------------------------------------

const string& Element::InputPath()
{
   static string DefaultInputPath("input");

   //  If our singleton hasn't even been created yet, InputPath_ might not
   //  have been initialized yet.  Return the static string created above.
   //
   auto element = Singleton< Element >::Extant();
   if(element == nullptr) return DefaultInputPath;
   return InputPath_;
}

//------------------------------------------------------------------------------

const string& Element::OutputPath()
{
   static string DefaultOutputPath("output");

   //  If our singleton hasn't even been created yet, OutputPath_ might not
   //  have been initialized yet.  Return the static string created above.
   //
   auto element = Singleton< Element >::Extant();
   if(element == nullptr) return DefaultOutputPath;
   return OutputPath_;
}

//------------------------------------------------------------------------------

void Element::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

string Element::strTimePlace()
{
   return (SysTime().to_str(SysTime::Alpha) + " on " + Name());
}
}
