//==============================================================================
//
//  MediaEndpt.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "MediaEndpt.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "MbPools.h"
#include "MediaPsm.h"
#include "MediaSsm.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace MediaBase
{
MediaEndpt::MediaEndpt(MediaPsm& psm) :
   psm_(&psm),
   state_(Idle)
{
   Debug::ft("MediaEndpt.ctor");

   //  Register with the PSM.
   //
   psm.SetMep(this);
}

//------------------------------------------------------------------------------

fn_name MediaEndpt_dtor = "MediaEndpt.dtor";

MediaEndpt::~MediaEndpt()
{
   Debug::ftnt(MediaEndpt_dtor);

   if(state_ != Idle)
   {
      Debug::SwLog(MediaEndpt_dtor, "unexpected state", state_);
   }

   //  Deregister from the PSM.
   //
   psm_->SetMep(nullptr);
}

//------------------------------------------------------------------------------

void MediaEndpt::Deallocate()
{
   Debug::ft("MediaEndpt.Deallocate");

   state_ = Idle;
}

//------------------------------------------------------------------------------

void MediaEndpt::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Pooled::Display(stream, prefix, options);

   stream << prefix << "psm : " << psm_ << CRLF;
}

//------------------------------------------------------------------------------

void MediaEndpt::EndOfTransaction()
{
   Debug::ft("MediaEndpt.EndOfTransaction");

   if(state_ == Idle) delete this;
}

//------------------------------------------------------------------------------

fn_name MediaEndpt_MgwPsm = "MediaEndpt.MgwPsm";

ProtocolSM* MediaEndpt::MgwPsm() const
{
   Debug::ft(MediaEndpt_MgwPsm);

   //  The media endpoint must be registered with a PSM.  This allows us to
   //  find the root SSM, which will be a media SSM that knows the identity
   //  of the media gateway PSM.
   //
   if(psm_ == nullptr)
   {
      Debug::SwLog(MediaEndpt_MgwPsm, "PSM not found", 0);
      return nullptr;
   }

   auto ssm = psm_->GetMediaSsm();

   if(ssm == nullptr) return nullptr;
   return ssm->MgwPsm();
}

//------------------------------------------------------------------------------

void* MediaEndpt::operator new(size_t size)
{
   Debug::ft("MediaEndpt.operator new");

   return Singleton<MediaEndptPool>::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

bool MediaEndpt::Passes(uint32_t selector) const
{
   return ((selector == 0) || (Psm()->GetFactory() == selector));
}

//------------------------------------------------------------------------------

void MediaEndpt::ProcessIcMsg(Message& msg)
{
   Debug::ft("MediaEndpt.ProcessIcMsg");

   //  This function must be overridden by subclasses that require it.
   //
   return;
}

//------------------------------------------------------------------------------

void MediaEndpt::SetState(StateId stid)
{
   Debug::ft("MediaEndpt.SetState");

   state_ = stid;
}
}
