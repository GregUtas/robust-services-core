//==============================================================================
//
//  SsmContext.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
#include "SbTrace.h"
#include "Singleton.h"
#include "SsmFactory.h"

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
   Debug::ft(SsmContext_dtor);

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
      Debug::SwErr
         (SsmContext_AllocRoot, pack2(header->protocol, header->signal), 0);
      return nullptr;
   }

   auto fid = header->rxAddr.fid;
   auto fac = Singleton < FactoryRegistry >::Instance()->GetFactory(fid);

   if(fac == nullptr)
   {
      Debug::SwErr
         (SsmContext_AllocRoot, pack2(header->protocol, header->signal), fid);
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

void SsmContext::GetSubtended(Base* objects[], size_t& count) const
{
   Debug::ft(SsmContext_GetSubtended);

   PsmContext::GetSubtended(objects, count);

   if(root_ != nullptr) root_->GetSubtended(objects, count);
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

   auto log = Log::Create("SERVICE ERROR");
   if(log == nullptr) return;
   *log << "sid=" << sid;
   *log << " errval=" << errval << CRLF;
   *log << "trace " << strTrace() << CRLF;
   Log::Spool(log);
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
   auto icEvent = port->ReceiveMsg(msg);

   if(icEvent != nullptr)
   {
      //  If the root SSM doesn't exist, create it.
      //
      if(root_ == nullptr)
      {
         root_ = AllocRoot(msg, *port->UppermostPsm());
         icEvent->SetOwner(*root_);
      }

      if(root_ != nullptr)
      {
         //  Keep processing events while the root SSM wishes to continue.
         //
         Event* ogEvent = nullptr;
         auto rc = EventHandler::Continue;

         while(rc == EventHandler::Continue)
         {
            rc = root_->ProcessEvent(icEvent, ogEvent);

            if(rc == EventHandler::Continue)
            {
               icEvent = ogEvent;
               ogEvent = nullptr;
            }
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
