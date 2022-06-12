//==============================================================================
//
//  MbPools.cpp
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
#include "MbPools.h"
#include <cstddef>
#include <iomanip>
#include <ostream>
#include <string>
#include <vector>
#include "Debug.h"
#include "Formatters.h"
#include "MediaEndpt.h"
#include "MediaPsm.h"
#include "NbAppIds.h"
#include "SbCliParms.h"
#include "SysTypes.h"
#include "ThisThread.h"

using std::ostream;
using std::setw;

//------------------------------------------------------------------------------

namespace MediaBase
{
constexpr size_t MediaEndptSize = sizeof(MediaEndpt) + (40 * BYTES_PER_WORD);

//------------------------------------------------------------------------------

MediaEndptPool::MediaEndptPool() :
   ObjectPool(MediaEndptObjPoolId, MemSlab, MediaEndptSize, "MediaEndpts")
{
   Debug::ft("MediaEndptPool.ctor");
}

//------------------------------------------------------------------------------

MediaEndptPool::~MediaEndptPool()
{
   Debug::ftnt("MediaEndptPool.dtor");
}

//------------------------------------------------------------------------------

fixed_string MepHeader = "Factory  State  Object";
//                       |      7      7..<object>

size_t MediaEndptPool::Summarize(ostream& stream, uint32_t selector) const
{
   stream << MepHeader << CRLF;

   auto items = GetUsed();

   if(items.empty())
   {
      stream << spaces(2) << NoMepsExpl << CRLF;
      return 0;
   }

   for(auto obj = items.cbegin(); obj != items.cend(); ++obj)
   {
      if((*obj)->IsValid() && (*obj)->Passes(selector))
      {
         auto mep = static_cast<const MediaEndpt*>(*obj);
         stream << setw(7) << mep->Psm()->GetFactory();
         stream << setw(7) << mep->GetState();
         stream << spaces(2) << strObj(this) << CRLF;
         ThisThread::PauseOver(90);
      }
   }

   return items.size();
}
}
