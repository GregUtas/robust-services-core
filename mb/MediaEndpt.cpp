//==============================================================================
//
//  MediaEndpt.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
fn_name MediaEndpt_ctor = "MediaEndpt.ctor";

MediaEndpt::MediaEndpt(MediaPsm& psm) :
   psm_(&psm),
   state_(Idle)
{
   Debug::ft(MediaEndpt_ctor);

   //  Register with the PSM.
   //
   psm.SetMep(this);
}

//------------------------------------------------------------------------------

fn_name MediaEndpt_dtor = "MediaEndpt.dtor";

MediaEndpt::~MediaEndpt()
{
   Debug::ft(MediaEndpt_dtor);

   if(state_ != Idle)
   {
      Debug::SwErr(MediaEndpt_dtor, state_, 0);
   }

   //  Deregister from the PSM.
   //
   psm_->SetMep(nullptr);
}

//------------------------------------------------------------------------------

fn_name MediaEndpt_Deallocate = "MediaEndpt.Deallocate";

void MediaEndpt::Deallocate()
{
   Debug::ft(MediaEndpt_Deallocate);

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

fn_name MediaEndpt_EndOfTransaction = "MediaEndpt.EndOfTransaction";

void MediaEndpt::EndOfTransaction()
{
   Debug::ft(MediaEndpt_EndOfTransaction);

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
      Debug::SwErr(MediaEndpt_MgwPsm, 0, 0);
      return nullptr;
   }

   auto ssm = psm_->GetMediaSsm();

   if(ssm == nullptr) return nullptr;
   return ssm->MgwPsm();
}

//------------------------------------------------------------------------------

fn_name MediaEndpt_new = "MediaEndpt.operator new";

void* MediaEndpt::operator new(size_t size)
{
   Debug::ft(MediaEndpt_new);

   return Singleton< MediaEndptPool >::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

fn_name MediaEndpt_ProcessIcMsg = "MediaEndpt.ProcessIcMsg";

void MediaEndpt::ProcessIcMsg(Message& msg)
{
   Debug::ft(MediaEndpt_ProcessIcMsg);

   //  This function must be overridden by subclasses that require it.
   //
   return;
}

//------------------------------------------------------------------------------

fn_name MediaEndpt_SetState = "MediaEndpt.SetState";

void MediaEndpt::SetState(StateId stid)
{
   Debug::ft(MediaEndpt_SetState);

   state_ = stid;
}
}
