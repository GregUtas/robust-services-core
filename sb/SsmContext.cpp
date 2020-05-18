//==============================================================================
//
//  SsmContext.cpp
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
#include "SsmContext.h"
#include <sstream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "Event.h"
#include "EventHandler.h"
#include "FactoryRegistry.h"
#include "LocalAddress.h"
#include "Log.h"
#include "Message.h"
#include "MsgHeader.h"
#include "MsgPort.h"
#include "RootServiceSM.h"
#include "SbLogs.h"
#include "SbTrace.h"
#include "Singleton.h"
#include "SsmFactory.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name SsmContext_ctor = "SsmContext.ctor";

SsmContext::SsmContext(Faction faction) : PsmContext(faction),
   root_(nullptr)
{
   Debug::ft(SsmContext_ctor);
}

//------------------------------------------------------------------------------

fn_name SsmContext_dtor = "SsmContext.dtor";

SsmContext::~SsmContext()
{
   Debug::ftnt(SsmContext_dtor);

   delete root_;
   root_ = nullptr;
}

//------------------------------------------------------------------------------

fn_name SsmContext_AllocRoot = "SsmContext.AllocRoot";

RootServiceSM* SsmContext::AllocRoot(const Message& msg, ProtocolSM& psm)
{
   Debug::ft(SsmContext_AllocRoot);

   //  In an SSM context, create the root SSM for the incoming message.
   //  This is done by delegating to the receiving factory.
   //
   auto header = msg.Header();

   if(!header->initial)
   {
      Debug::SwLog(SsmContext_AllocRoot, "initial message expected",
         pack2(header->protocol, header->signal));
      return nullptr;
   }

   auto fid = header->rxAddr.fid;
   auto fac = Singleton < FactoryRegistry >::Instance()->GetFactory(fid);

   if(fac == nullptr)
   {
      Debug::SwLog(SsmContext_AllocRoot, "factory not found",
         pack3(header->protocol, header->signal, fid));
      return nullptr;
   }

   return static_cast< SsmFactory* >(fac)->AllocRoot(msg, psm);
}

//------------------------------------------------------------------------------

void SsmContext::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   PsmContext::Display(stream, prefix, options);

   stream << prefix << "root : " << root_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name SsmContext_EndOfTransaction = "SsmContext.EndOfTransaction";

void SsmContext::EndOfTransaction()
{
   Debug::ft(SsmContext_EndOfTransaction);

   PsmContext::EndOfTransaction();

   if(root_ != nullptr) root_->EndOfTransaction();
}

//------------------------------------------------------------------------------

fn_name SsmContext_GetSubtended = "SsmContext.GetSubtended";

void SsmContext::GetSubtended(std::vector< Base* >& objects) const
{
   Debug::ft(SsmContext_GetSubtended);

   PsmContext::GetSubtended(objects);

   if(root_ != nullptr) root_->GetSubtended(objects);
}

//------------------------------------------------------------------------------

fn_name SsmContext_IsIdle = "SsmContext.IsIdle";

bool SsmContext::IsIdle() const
{
   Debug::ft(SsmContext_IsIdle);

   if(root_ == nullptr) return true;

   return PsmContext::IsIdle();
}

//------------------------------------------------------------------------------

fn_name SsmContext_OutputLog = "SsmContext.OutputLog";

void SsmContext::OutputLog(ServiceId sid, word errval) const
{
   Debug::ft(SsmContext_OutputLog);

   auto log = Log::Create(SessionLogGroup, ServiceError);
   if(log == nullptr) return;
   *log << Log::Tab << "sid=" << sid;
   *log << " errval=" << errval << CRLF;
   *log << Log::Tab << "trace " << strTrace();
   Log::Submit(log);
}

//------------------------------------------------------------------------------

void SsmContext::Patch(sel_t selector, void* arguments)
{
   PsmContext::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name SsmContext_ProcessIcMsg = "SsmContext.ProcessIcMsg";

void SsmContext::ProcessIcMsg(Message& msg)
{
   Debug::ft(SsmContext_ProcessIcMsg);

   //  Find or create the port that will receive MSG.
   //
   MsgPort* port = FindPort(msg);
   if(port == nullptr) return;

   //  Tell the port to process MSG.  This usually ends up returning an
   //  event for the root SSM.
   //
   auto currEvent = port->ReceiveMsg(msg);

   if(currEvent != nullptr)
   {
      //  If the root SSM doesn't exist, create it.
      //
      if(root_ == nullptr)
      {
         root_ = AllocRoot(msg, *port->UppermostPsm());
         currEvent->SetOwner(*root_);
      }

      //  Keep processing events while the root SSM wishes to continue.
      //
      Event* nextEvent = nullptr;
      auto rc = EventHandler::Continue;

      while(rc == EventHandler::Continue)
      {
         rc = root_->ProcessEvent(currEvent, nextEvent);

         if(rc == EventHandler::Continue)
         {
            currEvent = nextEvent;
            nextEvent = nullptr;
         }
      }
   }

   //  Tell the context's objects that the transaction is finished.
   //
   EndOfTransaction();

   if(root_ == nullptr) return;

   if(root_->CurrState() == ServiceSM::Null)
   {
      //  Delete the root SSM.  All PSMs and ports should have idled.
      //
      if((FirstPort() != nullptr) || (FirstPsm() != nullptr))
      {
         OutputLog(root_->Sid(), 0);
      }

      delete root_;
      root_ = nullptr;
   }
   else if((FirstPort() == nullptr) || (FirstPsm() == nullptr))
   {
      //  This root SSM is not idle but has no PSMs or no ports.
      //  This is a serious fault because it will no longer be
      //  able to receive messages.
      //
      OutputLog(root_->Sid(), 1);
      delete root_;
      root_ = nullptr;
   }
}

//------------------------------------------------------------------------------

fn_name SsmContext_SetRoot = "SsmContext.SetRoot";

void SsmContext::SetRoot(RootServiceSM* root)
{
   Debug::ft(SsmContext_SetRoot);

   root_ = root;

   if(root_ != nullptr)
   {
      auto trans = GetTrans();

      if(trans != nullptr) trans->SetService(root_->Sid());
   }
}
}
