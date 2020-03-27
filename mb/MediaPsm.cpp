//==============================================================================
//
//  MediaPsm.cpp
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
#include "MediaPsm.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Context.h"
#include "Debug.h"
#include "Formatters.h"
#include "MediaEndpt.h"
#include "MediaSsm.h"
#include "SbAppIds.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "TlvMessage.h"
#include "TlvParameter.h"
#include "ToneRegistry.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace MediaBase
{
fn_name MediaPsm_ctor1 = "MediaPsm.ctor(first)";

MediaPsm::MediaPsm(FactoryId fid) : ProtocolSM(fid),
   edge_(false),
   ogPsm_(nullptr),
   ogTone_(Tone::Silence),
   icTone_(Tone::Silence),
   mep_(nullptr)
{
   Debug::ft(MediaPsm_ctor1);
}

//------------------------------------------------------------------------------

fn_name MediaPsm_ctor2 = "MediaPsm.ctor(subseq)";

MediaPsm::MediaPsm(FactoryId fid, ProtocolLayer& adj, bool upper) :
   ProtocolSM(fid, adj, upper),
   edge_(false),
   ogPsm_(nullptr),
   ogTone_(Tone::Silence),
   icTone_(Tone::Silence),
   mep_(nullptr)
{
   Debug::ft(MediaPsm_ctor2);
}

//------------------------------------------------------------------------------

fn_name MediaPsm_dtor = "MediaPsm.dtor";

MediaPsm::~MediaPsm()
{
   Debug::ft(MediaPsm_dtor);

   //  Delete the media endpoint, if any.
   //
   delete mep_;
   mep_ = nullptr;
}

//------------------------------------------------------------------------------

fn_name MediaPsm_CalcIcPort = "MediaPsm.CalcIcPort";

Switch::PortId MediaPsm::CalcIcPort() const
{
   Debug::ft(MediaPsm_CalcIcPort);

   //  If the incoming tone is media, we listen to the port designated on the
   //  timeswitch.  If the incoming tone is something else, we listen to that
   //  specific tone, which overrides the port designated on the timeswitch.
   //
   if(icTone_ == Tone::Media) return icMedia_.rxFrom;
   return ToneRegistry::ToneToPort(icTone_);
}

//------------------------------------------------------------------------------

fn_name MediaPsm_CreateMedia = "MediaPsm.CreateMedia";

void MediaPsm::CreateMedia(MediaPsm& other)
{
   Debug::ft(MediaPsm_CreateMedia);

   SetOgPsm(&other);
   other.SetOgPsm(this);
   EnableMedia();
   other.EnableMedia();
}

//------------------------------------------------------------------------------

fn_name MediaPsm_DisableMedia = "MediaPsm.DisableMedia";

void MediaPsm::DisableMedia()
{
   Debug::ft(MediaPsm_DisableMedia);

   SetOgTone(Tone::Silence);
   SetIcTone(Tone::Silence);
}

//------------------------------------------------------------------------------

void MediaPsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   ProtocolSM::Display(stream, prefix, options);

   stream << prefix << "edge    : " << edge_ << CRLF;
   stream << prefix << "ogPsm   : " << ogPsm_ << CRLF;
   stream << prefix << "ogTone  : " << int(ogTone_) << CRLF;
   stream << prefix << "icTone  : " << int(icTone_) << CRLF;
   stream << prefix << "icMedia : " << CRLF;
   icMedia_.Display(stream, prefix + spaces(2));
   stream << prefix << "ogMediaSent : " << CRLF;
   ogMediaSent_.Display(stream, prefix + spaces(2));
   stream << prefix << "ogMediaCurr : " << CRLF;
   ogMediaCurr_.Display(stream, prefix + spaces(2));
   stream << prefix << "mep : " << mep_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name MediaPsm_EnableMedia1 = "MediaPsm.EnableMedia(other)";

void MediaPsm::EnableMedia(MediaPsm& other)
{
   Debug::ft(MediaPsm_EnableMedia1);

   SetOgPsm(&other);
   other.SetOgPsm(this);
   EnableMedia();
}

//------------------------------------------------------------------------------

fn_name MediaPsm_EnableMedia2 = "MediaPsm.EnableMedia";

void MediaPsm::EnableMedia()
{
   Debug::ft(MediaPsm_EnableMedia2);

   SetOgTone(Tone::Media);
   SetIcTone(Tone::Media);
}

//------------------------------------------------------------------------------

fn_name MediaPsm_EndOfTransaction = "MediaPsm.EndOfTransaction";

void MediaPsm::EndOfTransaction()
{
   Debug::ft(MediaPsm_EndOfTransaction);

   //  Give our MEP a chance to generate a media message or parameter.
   //  This occurs first so that the MEP's contribution can be included
   //  in a larger message built by PrepareOgMsgq or ProcessOgMsg.
   //
   if(mep_ != nullptr) mep_->EndOfTransaction();

   ProtocolSM::EndOfTransaction();
}

//------------------------------------------------------------------------------

fn_name MediaPsm_EnsureMedia = "MediaPsm.EnsureMedia";

void MediaPsm::EnsureMedia(MediaPsm& other)
{
   Debug::ft(MediaPsm_EnsureMedia);

   //  If neither this PSM nor PEER have a peer media PSM, prepare to
   //  set up media between them.
   //
   if((GetOgPsm() == nullptr) && (other.GetOgPsm() == nullptr))
   {
      SetOgPsm(&other);
      other.SetOgPsm(this);
   }

   //  Enable media on PSM and its peer.
   //
   EnableMedia();
   auto peer = GetOgPsm();
   if(peer != nullptr) peer->EnableMedia();

   //  If this PSM's peer is not OTHER, enable media on OTHER and its peer.
   //
   if(peer != &other)
   {
      other.EnableMedia();
      peer = other.GetOgPsm();
      if(peer != nullptr) peer->EnableMedia();
   }
}

//------------------------------------------------------------------------------

fn_name MediaPsm_EnsureMediaMsg = "MediaPsm.EnsureMediaMsg";

void MediaPsm::EnsureMediaMsg()
{
   Debug::ft(MediaPsm_EnsureMediaMsg);

   Context::Kill(strOver(this), GetFactory());
}

//------------------------------------------------------------------------------

fn_name MediaPsm_GetMediaSsm = "MediaPsm.GetMediaSsm";

MediaSsm* MediaPsm::GetMediaSsm() const
{
   Debug::ft(MediaPsm_GetMediaSsm);

   auto root = RootSsm();

   if(root == nullptr)
   {
      Debug::SwLog(MediaPsm_GetMediaSsm, "root SSM not found", 0);
      return nullptr;
   }

   if(root->Sid() != TestServiceId) return static_cast< MediaSsm* >(root);
   return nullptr;
}

//------------------------------------------------------------------------------

MediaPsm* MediaPsm::GetOgPsm() const
{
   return ogPsm_;
}

//------------------------------------------------------------------------------

fn_name MediaPsm_GetSubtended = "MediaPsm.GetSubtended";

void MediaPsm::GetSubtended(Base* objects[], size_t& count) const
{
   Debug::ft(MediaPsm_GetSubtended);

   ProtocolSM::GetSubtended(objects, count);

   if(mep_ != nullptr) mep_->GetSubtended(objects, count);
}

//------------------------------------------------------------------------------

fn_name MediaPsm_IcPortUpdated = "MediaPsm.IcPortUpdated";

void MediaPsm::IcPortUpdated() const
{
   Debug::ft(MediaPsm_IcPortUpdated);

   //  If we are transmitting media (as opposed to a fixed tone), tell all
   //  of our listeners that they must now listen to a different port.
   //
   if(icTone_ == Tone::Media)
   {
      auto ssm = GetMediaSsm();
      if(ssm != nullptr) ssm->NotifyListeners(*this, CalcIcPort());
   }
}

//------------------------------------------------------------------------------

fn_name MediaPsm_MakeEdge = "MediaPsm.MakeEdge";

void MediaPsm::MakeEdge(Switch::PortId port)
{
   Debug::ft(MediaPsm_MakeEdge);

   //  If this is not currently an edge PSM, see if the port from which
   //  it is transmitting has changed.
   //  listen.
   //
   if(!edge_)
   {
      edge_ = true;

      if(icMedia_.rxFrom != port)
      {
         icMedia_.rxFrom = port;

         //  This PSM has a new port.  If the PSM has not idled, notify its
         //  listeners.  If the new port is not listening to what this PSM
         //  is supposed to be receiving, update its connection and ensure
         //  that it sends a media update at the end of the transaction.
         //  The latter is done by invalidating the media information that
         //  was previously sent.
         //
         if(GetState() != Idle)
         {
            IcPortUpdated();

            auto tsw = Singleton< Switch >::Instance();
            auto cct = tsw->GetCircuit(icMedia_.rxFrom);

            if(cct->RxFrom() != ogMediaCurr_.rxFrom)
            {
               cct->MakeConn(ogMediaCurr_.rxFrom);
               ogMediaSent_.rxFrom = NIL_ID;
            }
         }
      }
   }
}

//------------------------------------------------------------------------------

fn_name MediaPsm_MakeRelay = "MediaPsm.MakeRelay";

void MediaPsm::MakeRelay()
{
   Debug::ft(MediaPsm_MakeRelay);

   edge_ = false;
}

//------------------------------------------------------------------------------

fn_name MediaPsm_PrepareOgMsgq = "MediaPsm.PrepareOgMsgq";

void MediaPsm::PrepareOgMsgq()
{
   Debug::ft(MediaPsm_PrepareOgMsgq);

   //  If a media update is required, make sure that the outgoing message
   //  queue will have a message in which that parameter can be included.
   //
   if(ogMediaSent_ != ogMediaCurr_) EnsureMediaMsg();
}

//------------------------------------------------------------------------------

fn_name MediaPsm_ReceiveMsg = "MediaPsm.ReceiveMsg";

Event* MediaPsm::ReceiveMsg(Message& msg)
{
   Debug::ft(MediaPsm_ReceiveMsg);

   //  Give the MEP a chance to process any media parameters before
   //  application software handles the message.  This ensure that
   //  media modifications ripple through the context before the end
   //  of the transaction.
   //
   if(mep_ != nullptr) mep_->ProcessIcMsg(msg);

   return ProtocolSM::ReceiveMsg(msg);
}

//------------------------------------------------------------------------------

fn_name MediaPsm_SetIcTone = "MediaPsm.SetIcTone";

void MediaPsm::SetIcTone(Tone::Id icTone)
{
   Debug::ft(MediaPsm_SetIcTone);

   Switch::PortId port;

   //  If the tone has changed, tell the PSM's listeners that they should
   //  now listen to a different port.  If the tone is media, they listen
   //  to the PSM's underlying port, else they listen to the tone specified.
   //
   if(icTone_ == icTone) return;

   icTone_ = icTone;

   if(icTone == Tone::Media)
      port = icMedia_.rxFrom;
   else
      port = ToneRegistry::ToneToPort(icTone);

   auto ssm = GetMediaSsm();
   if(ssm != nullptr) ssm->NotifyListeners(*this, port);
}

//------------------------------------------------------------------------------

fn_name MediaPsm_SetMep = "MediaPsm.SetMep";

void MediaPsm::SetMep(MediaEndpt* mep)
{
   Debug::ft(MediaPsm_SetMep);

   mep_ = mep;
}

//------------------------------------------------------------------------------

fn_name MediaPsm_SetOgPort = "MediaPsm.SetOgPort";

void MediaPsm::SetOgPort(Switch::PortId ogport)
{
   Debug::ft(MediaPsm_SetOgPort);

   //  The outgoing port is the one to which the PSM is listening.  When it
   //  changes, the timeswitch connection must be updated if the PSM is an
   //  edge PSM (which means that it owns an actual circuit).  For both edge
   //  and relay PSMs, the port's address is sent in a media parameter that
   //  is added to the next outgoing message.
   //
   if(ogMediaCurr_.rxFrom != ogport)
   {
      ogMediaCurr_.rxFrom = ogport;

      if(edge_)
      {
         auto tsw = Singleton< Switch >::Instance();
         auto cct = tsw->GetCircuit(icMedia_.rxFrom);

         if(cct != nullptr) cct->MakeConn(ogMediaCurr_.rxFrom);
      }
   }
}

//------------------------------------------------------------------------------

fn_name MediaPsm_SetOgPsm = "MediaPsm.SetOgPsm";

void MediaPsm::SetOgPsm(MediaPsm* ogPsm)
{
   Debug::ft(MediaPsm_SetOgPsm);

   //  In a protocol stack, this function is restricted to the uppermost PSM.
   //
   if(!IsUppermost())
   {
      Debug::SwLog(MediaPsm_SetOgPsm, "not uppermost PSM", GetFactory());
      return;
   }

   //  If this PSM has changed its media source, and if it is listening to
   //  media rather than a fixed tone, calculate the port to which it should
   //  now listen.
   //
   if(ogPsm_ == ogPsm) return;

   ogPsm_ = ogPsm;

   if(ogTone_ == Tone::Media)
   {
      if(ogPsm != nullptr)
         SetOgPort(ogPsm->CalcIcPort());
      else
         SetOgPort(Switch::SilentPort);
   }
}

//------------------------------------------------------------------------------

fn_name MediaPsm_SetOgTone = "MediaPsm.SetOgTone";

void MediaPsm::SetOgTone(Tone::Id ogTone)
{
   Debug::ft(MediaPsm_SetOgTone);

   //  In a protocol stack, this function is restricted to the uppermost PSM.
   //
   if(!IsUppermost())
   {
      Debug::SwLog(MediaPsm_SetOgTone, "not uppermost PSM", GetFactory());
      return;
   }

   //  If the tone that the PSM should listen to has changed, find the
   //  port from which the PSM should now receive.
   //
   if(ogTone_ == ogTone) return;

   ogTone_ = ogTone;

   if(ogTone_ == Tone::Media)
   {
      if(ogPsm_ == nullptr)
         SetOgPort(Switch::SilentPort);
      else
         SetOgPort(ogPsm_->CalcIcPort());
   }
   else
   {
      SetOgPort(ToneRegistry::ToneToPort(ogTone));
   }
}

//------------------------------------------------------------------------------

fn_name MediaPsm_SynchEdge = "MediaPsm.SynchEdge";

void MediaPsm::SynchEdge(MediaPsm& psm) const
{
   Debug::ft(MediaPsm_SynchEdge);

   //  To synchronize an edge PSM, update what it is receiving (ogMedia_).
   //  What it is transmitting (icMedia_) is not updated, because this is
   //  fixed by the port assigned the underlying circuit.
   //
   psm.ogMediaSent_ = ogMediaSent_;
   psm.ogMediaCurr_ = ogMediaCurr_;

   if(ogMediaSent_ != ogMediaCurr_)
   {
      //  PSM should be in a stable state, so this is a problem.
      //
      Debug::SwLog(MediaPsm_SynchEdge, "media not in synch",
         pack2(psm.GetFactory(), GetFactory()));
   }
}

//------------------------------------------------------------------------------

fn_name MediaPsm_SynchRelay = "MediaPsm.SynchRelay";

void MediaPsm::SynchRelay(MediaPsm& psm) const
{
   Debug::ft(MediaPsm_SynchRelay);

   //  To synchronize a relay PSM, update both what it is receiving and
   //  transmitting.
   //
   psm.ogMediaSent_ = icMedia_;
   psm.ogMediaCurr_ = icMedia_;
   psm.icMedia_ = ogMediaSent_;
}

//------------------------------------------------------------------------------

fn_name MediaPsm_UpdateIcMedia = "MediaPsm.UpdateIcMedia";

void MediaPsm::UpdateIcMedia(TlvMessage& msg, ParameterId pid)
{
   Debug::ft(MediaPsm_UpdateIcMedia);

   //  An edge PSM does not update the port from which it transmits, because
   //  it is always the port assigned to its underlying circuit.  A relay PSM,
   //  however, changes this port when its PSM listens to another PSM or when
   //  incoming and outgoing tones are modified.
   //
   if(edge_) return;

   auto pptr = msg.FindParm(pid);

   if(pptr != nullptr)
   {
      auto cxi = reinterpret_cast< MediaInfo* >(pptr->bytes);

      if(icMedia_.rxFrom != cxi->rxFrom)
      {
         icMedia_ = *cxi;
         IcPortUpdated();
      }

      //  The incoming media parameter has been handled, so delete it.
      //  This prevents it from being relayed, which would bypass this
      //  context's media configuration.
      //
      msg.DeleteParm(*pptr);
   }
}

//------------------------------------------------------------------------------

fn_name MediaPsm_UpdateOgMedia = "MediaPsm.UpdateOgMedia";

void MediaPsm::UpdateOgMedia(TlvMessage& msg, ParameterId pid)
{
   Debug::ft(MediaPsm_UpdateOgMedia);

   if(ogMediaSent_ != ogMediaCurr_)
   {
      msg.AddType(ogMediaCurr_, pid);
      ogMediaSent_ = ogMediaCurr_;
   }
}
}
