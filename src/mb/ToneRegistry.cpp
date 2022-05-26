//==============================================================================
//
//  ToneRegistry.cpp
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
#include "ToneRegistry.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace MediaBase
{
ToneRegistry::ToneRegistry()
{
   Debug::ft("ToneRegistry.ctor");

   tones_.Init(Tone::MaxId, Tone::CellDiff(), MemDynamic);
}

//------------------------------------------------------------------------------

fn_name ToneRegistry_dtor = "ToneRegistry.dtor";

ToneRegistry::~ToneRegistry()
{
   Debug::ftnt(ToneRegistry_dtor);

   Debug::SwLog(ToneRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

bool ToneRegistry::BindTone(Tone& tone)
{
   Debug::ft("ToneRegistry.BindTone");

   return tones_.Insert(tone);
}

//------------------------------------------------------------------------------

void ToneRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "tones [Tone::Id]" << CRLF;
   tones_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

Tone* ToneRegistry::GetTone(Tone::Id tid) const
{
   return tones_.At(tid);
}

//------------------------------------------------------------------------------

Switch::PortId ToneRegistry::ToneToPort(Tone::Id tid)
{
   Debug::ft("ToneRegistry.ToneToPort");

   auto tone = Singleton<ToneRegistry>::Instance()->GetTone(tid);
   if(tone != nullptr) return tone->TsPort();
   return NIL_ID;
}

//------------------------------------------------------------------------------

void ToneRegistry::UnbindTone(Tone& tone)
{
   Debug::ftnt("ToneRegistry.UnbindTone");

   tones_.Erase(tone);
}
}
