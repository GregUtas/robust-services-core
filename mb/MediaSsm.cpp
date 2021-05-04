//==============================================================================
//
//  MediaSsm.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "MediaSsm.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "MediaPsm.h"
#include "SsmContext.h"
#include "SysTypes.h"
#include "Tones.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace MediaBase
{
MediaSsm::MediaSsm(ServiceId sid) : RootServiceSM(sid),
   mgwPsm_(nullptr)
{
   Debug::ft("MediaSsm.ctor");
}

//------------------------------------------------------------------------------

MediaSsm::~MediaSsm()
{
   Debug::ftnt("MediaSsm.dtor");
}

//------------------------------------------------------------------------------

void MediaSsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   RootServiceSM::Display(stream, prefix, options);

   stream << prefix << "mgwPsm : " << mgwPsm_ << CRLF;
}

//------------------------------------------------------------------------------

void MediaSsm::GetSubtended(std::vector< Base* >& objects) const
{
   Debug::ft("MediaSsm.GetSubtended");

   RootServiceSM::GetSubtended(objects);

   if(mgwPsm_ != nullptr) mgwPsm_->GetSubtended(objects);
}

//------------------------------------------------------------------------------

void MediaSsm::NotifyListeners
   (const ProtocolSM& txPsm, Switch::PortId txPort) const
{
   Debug::ft("MediaSsm.NotifyListeners");

   //  Notify all of the PSMs that are listening to txPsm that they should
   //  now listen to txPort.
   //
   auto ctx = GetContext();

   for(auto psm = ctx->FirstPsm(); psm != nullptr; ctx->NextPsm(psm))
   {
      auto mpsm = static_cast< MediaPsm* >(psm);

      if((mpsm->GetOgPsm() == &txPsm) && (mpsm->GetOgTone() == Tone::Media))
      {
         mpsm->SetOgPort(txPort);
      }
   }
}

//------------------------------------------------------------------------------

void MediaSsm::PsmDeleted(ProtocolSM& exPsm)
{
   Debug::ft("MediaSsm.PsmDeleted");

   //  Notify all of the PSMs that are listening to exPsm that exPsm has
   //  been deleted.
   //
   auto ctx = GetContext();

   for(auto psm = ctx->FirstPsm(); psm != nullptr; ctx->NextPsm(psm))
   {
      auto mpsm = static_cast< MediaPsm* >(psm);

      if(mpsm->GetOgPsm() == &exPsm)
      {
         //  This often occurs at the end of a transaction, in which case
         //  ProcessOgMsg has already been invoked on MPSM if it precedes
         //  exPsm in the SSM's PSM queue.  It is then* too late* for MPSM
         //  to send its MediaInfo parameter during this transaction.  An
         //  application must invoke exPsm.SetIcTone(Tone::Silence) during
         //  the transaction to avoid this type of problem.
         //
         mpsm->SetOgPsm(nullptr);
      }
   }

   if(mgwPsm_ == &exPsm) mgwPsm_ = nullptr;

   RootServiceSM::PsmDeleted(exPsm);
}

//------------------------------------------------------------------------------

fn_name MediaSsm_SetMgwPsm = "MediaSsm.SetMgwPsm";

bool MediaSsm::SetMgwPsm(ProtocolSM* psm)
{
   Debug::ft(MediaSsm_SetMgwPsm);

   if(psm != nullptr)
   {
      if(mgwPsm_ == nullptr)
      {
         mgwPsm_ = psm;
         return true;
      }

      //  A media gateway PSM already exists.
      //
      Debug::SwLog(MediaSsm_SetMgwPsm, "PSM already exists", 1);
   }
   else
   {
      if(mgwPsm_ != nullptr)
      {
         mgwPsm_ = nullptr;
         return true;
      }

      //  There wasn't a media gateway PSM to free.
      //
      Debug::SwLog(MediaSsm_SetMgwPsm, "no PSM exists", 0);
   }

   return false;
}
}
