//==============================================================================
//
//  BotTrace.cpp
//
//  Copyright (C) 2017  Greg Utas
//
//  Diplomacy AI Client - Part of the DAIDE project (www.daide.org.uk).
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
#include "BotTrace.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "DipProtocol.h"
#include "DipTypes.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace Diplomacy
{
BotTrace::BotTrace(Id rid, const DipIpBuffer& buff) :
   TimedRecord(DipTracer),
   buff_(nullptr),
   corrupt_(false)
{
   buff_ = new DipIpBuffer(buff);
   rid_ = rid;
}

//------------------------------------------------------------------------------

BotTrace::~BotTrace()
{
   //  If our DipIpBuffer is corrupt, we will trap, and we must not trap again
   //  during cleanup.  Flag the buffer as corrupt before deleting it and
   //  clear the flag afterwards.  If it flagged as corrupt when we come in
   //  here, we know that it was bad, so skip it and let the audit find it.
   //
   if((buff_ != nullptr) && !buff_->IsInvalid())
   {
      if(!corrupt_)
      {
         corrupt_ = true;
         delete buff_;
      }

      buff_ = nullptr;
      corrupt_ = false;
   }
}

//------------------------------------------------------------------------------

fn_name BotTrace_ClaimBlocks = "BotTrace.ClaimBlocks";

void BotTrace::ClaimBlocks()
{
   Debug::ft(BotTrace_ClaimBlocks);

   if((buff_ != nullptr) && !corrupt_ && !buff_->IsInvalid())
   {
      buff_->ClaimBlocks();
   }
}

//------------------------------------------------------------------------------

bool BotTrace::Display(ostream& stream, const string& opts)
{
   if(!TimedRecord::Display(stream, opts)) return false;

   stream << CRLF;
   stream << string(COUT_LENGTH_MAX, '-') << CRLF;

   if(buff_ == nullptr)
   {
      stream << "No buffer found." << CRLF;
      stream << string(COUT_LENGTH_MAX, '-');
      return true;
   }

   if(!buff_->IsInvalid())
   {
      auto message = reinterpret_cast< const DipMessage* >(buff_->HeaderPtr());
      message->Display(stream);
   }

   stream << string(COUT_LENGTH_MAX, '-');
   return true;
}

//------------------------------------------------------------------------------

fixed_string IcMsgEventStr = "icmsg";
fixed_string OgMsgEventStr = "ogmsg";

c_string BotTrace::EventString() const
{
   switch(rid_)
   {
   case IcMsg: return IcMsgEventStr;
   case OgMsg: return OgMsgEventStr;
   }

   return ERROR_STR;
}

//------------------------------------------------------------------------------

DipHeader* BotTrace::Header() const
{
   if(buff_ == nullptr) return nullptr;
   return reinterpret_cast< DipHeader* >(buff_->HeaderPtr());
}

//------------------------------------------------------------------------------

fn_name BotTrace_Shutdown = "BotTrace.Shutdown";

void BotTrace::Shutdown(RestartLevel level)
{
   Debug::ft(BotTrace_Shutdown);

   if(level >= RestartCold) Nullify();
}
}
