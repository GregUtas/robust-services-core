//==============================================================================
//
//  Factory.cpp
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
#include "Factory.h"
#include "Dynamic.h"
#include <cstdint>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "FactoryRegistry.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "Restart.h"
#include "Singleton.h"
#include "Statistics.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Statistics for each factory.
//
class FactoryStats : public Dynamic
{
public:
   FactoryStats();
   ~FactoryStats();
   FactoryStats(const FactoryStats& that) = delete;
   FactoryStats& operator=(const FactoryStats& that) = delete;

   CounterPtr       icMsgsIntra_;
   CounterPtr       icMsgsInter_;
   HighWatermarkPtr icMsgSize_;
   CounterPtr       ogMsgsIntra_;
   CounterPtr       ogMsgsInter_;
   HighWatermarkPtr ogMsgSize_;
   CounterPtr       contexts_;
   CounterPtr       msgsDeleted_;
   CounterPtr       ctxsDeleted_;
};

//------------------------------------------------------------------------------

FactoryStats::FactoryStats()
{
   Debug::ft("FactoryStats.ctor");

   icMsgsIntra_.reset(new Counter("incoming intraprocessor messages"));
   icMsgsInter_.reset(new Counter("incoming interprocessor messages"));
   icMsgSize_.reset(new HighWatermark("longest incoming message"));
   ogMsgsIntra_.reset(new Counter("outgoing intraprocessor messages"));
   ogMsgsInter_.reset(new Counter("outgoing interprocessor messages"));
   ogMsgSize_.reset(new HighWatermark("longest outgoing message"));
   contexts_.reset(new Counter("contexts created"));
   msgsDeleted_.reset(new Counter("retransmitted messages deleted"));
   ctxsDeleted_.reset(new Counter("contexts freed on request-cancel"));
}

//------------------------------------------------------------------------------

fn_name FactoryStats_dtor = "FactoryStats.dtor";

FactoryStats::~FactoryStats()
{
   Debug::ftnt(FactoryStats_dtor);

   Debug::SwLog(FactoryStats_dtor, UnexpectedInvocation, 0);
}

//==============================================================================

Factory::Factory(Id fid, ContextType type, ProtocolId prid, c_string name) :
   type_(type),
   faction_(PayloadFaction),
   prid_(prid),
   name_(name),
   icSignals_{false},
   ogSignals_{false}
{
   Debug::ft("Factory.ctor");

   Debug::Assert(name_ != nullptr);

   fid_.SetId(fid);
   stats_.reset(new FactoryStats);

   //  Add the factory to the global factory registry.
   //
   Singleton<FactoryRegistry>::Instance()->BindFactory(*this);
}

//------------------------------------------------------------------------------

fn_name Factory_dtor = "Factory.dtor";

Factory::~Factory()
{
   Debug::ftnt(Factory_dtor);

   Debug::SwLog(Factory_dtor, UnexpectedInvocation, 0);
   Singleton<FactoryRegistry>::Extant()->UnbindFactory(*this);
}

//------------------------------------------------------------------------------

fn_name Factory_AddIncomingSignal = "Factory.AddIncomingSignal";

void Factory::AddIncomingSignal(SignalId sid)
{
   Debug::ft(Factory_AddIncomingSignal);

   if(!Signal::IsValidId(sid))
   {
      Debug::SwLog(Factory_AddIncomingSignal, "invalid signal", sid);
      return;
   }

   icSignals_[sid] = true;
}

//------------------------------------------------------------------------------

fn_name Factory_AddOutgoingSignal = "Factory.AddOutgoingSignal";

void Factory::AddOutgoingSignal(SignalId sid)
{
   Debug::ft(Factory_AddOutgoingSignal);

   if(!Signal::IsValidId(sid))
   {
      Debug::SwLog(Factory_AddOutgoingSignal, "invalid signal", sid);
      return;
   }

   ogSignals_[sid] = true;
}

//------------------------------------------------------------------------------

fn_name Factory_AllocContext = "Factory.AllocContext";

Context* Factory::AllocContext() const
{
   Debug::ft(Factory_AllocContext);

   Debug::SwLog(Factory_AllocContext, strOver(this), Fid());
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Factory_AllocIcMsg = "Factory.AllocIcMsg";

Message* Factory::AllocIcMsg(SbIpBufferPtr& buff) const
{
   Debug::ft(Factory_AllocIcMsg);

   Debug::SwLog(Factory_AllocIcMsg, strOver(this), Fid());
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Factory_AllocOgMsg = "Factory.AllocOgMsg";

Message* Factory::AllocOgMsg(SignalId sid) const
{
   Debug::ft(Factory_AllocOgMsg);

   Debug::SwLog(Factory_AllocOgMsg, strOver(this), Fid());
   return nullptr;
}

//------------------------------------------------------------------------------

ptrdiff_t Factory::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const Factory*>(&local);
   return ptrdiff(&fake->fid_, fake);
}

//------------------------------------------------------------------------------

CliText* Factory::CreateText() const
{
   Debug::ft("Factory.CreateText");

   return nullptr;
}

//------------------------------------------------------------------------------

size_t Factory::DiscardedContextCount() const
{
   return stats_->ctxsDeleted_->Curr();
}

//------------------------------------------------------------------------------

size_t Factory::DiscardedMessageCount() const
{
   return stats_->msgsDeleted_->Curr();
}

//------------------------------------------------------------------------------

void Factory::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "fid     : " << fid_.to_str() << CRLF;
   stream << prefix << "type    : " << int(type_) << CRLF;
   stream << prefix << "faction : " << int(faction_) << CRLF;
   stream << prefix << "prid    : " << prid_ << CRLF;
   stream << prefix << "name    : " << name_ << CRLF;
   stream << prefix << "icSignals : ";

   for(size_t i = 0; i <= Signal::MaxId; ++i)
   {
      if(icSignals_[i]) stream << int(i) << SPACE;
   }

   stream << CRLF;
   stream << prefix << "ogSignals : ";

   for(size_t i = 0; i <= Signal::MaxId; ++i)
   {
      if(ogSignals_[i]) stream << int(i) << SPACE;
   }

   stream << CRLF;
}

//------------------------------------------------------------------------------

void Factory::DisplayStats(ostream& stream, const Flags& options) const
{
   Debug::ft("Factory.DisplayStats");

   stream << spaces(2) << name_ << SPACE << strIndex(Fid(), 0, false) << CRLF;

   stats_->icMsgsIntra_->DisplayStat(stream, options);
   stats_->icMsgsInter_->DisplayStat(stream, options);
   stats_->icMsgSize_->DisplayStat(stream, options);
   stats_->ogMsgsIntra_->DisplayStat(stream, options);
   stats_->ogMsgsInter_->DisplayStat(stream, options);
   stats_->ogMsgSize_->DisplayStat(stream, options);
   stats_->contexts_->DisplayStat(stream, options);
   stats_->msgsDeleted_->DisplayStat(stream, options);
   stats_->ctxsDeleted_->DisplayStat(stream, options);
}

//------------------------------------------------------------------------------

void Factory::IncrContexts() const
{
   Debug::ft("Factory.IncrContexts");

   stats_->contexts_->Incr();
}

//------------------------------------------------------------------------------

fn_name Factory_InjectMsg = "Factory.InjectMsg";

bool Factory::InjectMsg(Message& msg) const
{
   Debug::ft(Factory_InjectMsg);

   Debug::SwLog(Factory_InjectMsg, strOver(this), Fid());
   return false;
}

//------------------------------------------------------------------------------

bool Factory::IsLegalIcSignal(SignalId sid) const
{
   Debug::ft("Factory.IsLegalIcSignal");

   if(!Signal::IsValidId(sid)) return false;
   return icSignals_[sid];
}

//------------------------------------------------------------------------------

bool Factory::IsLegalOgSignal(SignalId sid) const
{
   Debug::ft("Factory.IsLegalOgSignal");

   if(!Signal::IsValidId(sid)) return false;
   return ogSignals_[sid];
}

//------------------------------------------------------------------------------

void Factory::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Factory_ReallocOgMsg = "Factory.ReallocOgMsg";

Message* Factory::ReallocOgMsg(SbIpBufferPtr& buff) const
{
   Debug::ft(Factory_ReallocOgMsg);

   Debug::SwLog(Factory_ReallocOgMsg, strOver(this), Fid());
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Factory_ReceiveMsg = "Factory.ReceiveMsg";

Factory::Rc Factory::ReceiveMsg
   (Message& msg, bool atIoLevel, TransTrace* tt, Context*& ctx)
{
   Debug::ft(Factory_ReceiveMsg);

   Debug::SwLog(Factory_ReceiveMsg, strOver(this), Fid());
   return FactoryNotReceiving;
}

//------------------------------------------------------------------------------

void Factory::RecordDeletion(bool context) const
{
   Debug::ft("Factory.RecordDeletion");

   if(context)
      stats_->ctxsDeleted_->Incr();
   else
      stats_->msgsDeleted_->Incr();
}

//------------------------------------------------------------------------------

void Factory::RecordMsg(bool incoming, bool inter, size_t size) const
{
   Debug::ft("Factory.RecordMsg");

   if(incoming)
   {
      if(inter)
         stats_->icMsgsInter_->Incr();
      else
         stats_->icMsgsIntra_->Incr();

      stats_->icMsgSize_->Update(size);
   }
   else
   {
      if(inter)
         stats_->ogMsgsInter_->Incr();
      else
         stats_->ogMsgsIntra_->Incr();

      stats_->ogMsgSize_->Update(size);
   }
}

//------------------------------------------------------------------------------

bool Factory::ScreenFirstMsg(const Message& msg, MsgPriority& prio) const
{
   Debug::ft("Factory.ScreenFirstMsg");

   return false;
}

//------------------------------------------------------------------------------

bool Factory::ScreenIcMsgs(Q1Way<Message>& msgq)
{
   Debug::ft("Factory.ScreenIcMsgs");

   return true;
}

//------------------------------------------------------------------------------

void Factory::Shutdown(RestartLevel level)
{
   Debug::ft("Factory.Shutdown");

   FunctionGuard guard(Guard_ImmUnprotect);
   Restart::Release(stats_);
}

//------------------------------------------------------------------------------

void Factory::Startup(RestartLevel level)
{
   Debug::ft("Factory.Startup");

   if(stats_ == nullptr)
   {
      FunctionGuard guard(Guard_ImmUnprotect);
      stats_.reset(new FactoryStats);
   }
}
}
