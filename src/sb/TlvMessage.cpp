//==============================================================================
//
//  TlvMessage.cpp
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
#include "TlvMessage.h"
#include "Algorithms.h"
#include "Context.h"
#include "MsgPort.h"
#include "NbTypes.h"
#include "ProtocolSM.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
TlvMessage::TlvMessage(SbIpBufferPtr& buff) : Message(buff)
{
   Debug::ft("TlvMessage.ctor(i/c)");
}

//------------------------------------------------------------------------------

TlvMessage::TlvMessage(ProtocolSM* psm, size_t size) :
   Message(psm, Pad(size) + FenceSize)
{
   Debug::ft("TlvMessage.ctor(o/g)");

   //  An outgoing TLV message must always end with the parameter fence.
   //
   *FencePtr() = ParmFencePattern;
}

//------------------------------------------------------------------------------

TlvMessage::TlvMessage(const TlvParm& parm, ProtocolSM* psm) :
   Message(psm, parm.header.plen)
{
   Debug::ft("TlvMessage.ctor(unwrap)");

   //  We just constructed an empty outgoing message.  Fill it with the
   //  message encapsulated in PARM and make it an incoming message.
   //
   byte_t* parms;
   Payload(parms);
   auto encap = reinterpret_cast<const TlvMsgLayout*>(parm.bytes);

   //  Unbundle the header and payload.
   //
   *Header() = encap->header;
   Memory::Copy(parms, encap->bytes, encap->header.length);
   ChangeDir(MsgIncoming);
   SetReceiver(psm->Port()->LocAddr());
   SetSender(psm->Port()->RemAddr());
}

//------------------------------------------------------------------------------

TlvMessage::TlvMessage(const Message& msg, ProtocolSM* psm) :
   Message(psm, msg.Header()->length + FenceSize)
{
   Debug::ft("TlvMessage.ctor(copy)");

   byte_t* from;
   byte_t* to;

   //  We've constructed an empty outgoing message.  Fill it with
   //  MSG's contents, set its length, and append its fence.
   //
   auto size = msg.Header()->length;
   msg.Payload(from);
   Payload(to);
   Memory::Copy(to, from, size);
   Header()->length = size;
   *FencePtr() = ParmFencePattern;
}

//------------------------------------------------------------------------------

TlvMessage::~TlvMessage()
{
   Debug::ftnt("TlvMessage.dtor");
}

//------------------------------------------------------------------------------

TlvParm* TlvMessage::AddBytes(const byte_t* src, size_t size, ParameterId pid)
{
   Debug::ft("TlvMessage.AddBytes");

   auto pptr = AddParm(pid, size);
   if(pptr != nullptr) Memory::Copy(pptr->bytes, src, size);
   return pptr;
}

//------------------------------------------------------------------------------

void TlvMessage::AddFence()
{
   Debug::ft("TlvMessage.AddFence");

   bool moved = false;
   if(!WriteBuffer()->AddBytes(nullptr, FenceSize, moved)) return;
   if(moved) Refresh();
   *FencePtr() = ParmFencePattern;
}

//------------------------------------------------------------------------------

fn_name TlvMessage_AddParm = "TlvMessage.AddParm";

TlvParm* TlvMessage::AddParm(ParameterId pid, size_t plen)
{
   Debug::ft(TlvMessage_AddParm);

   //  Prevent modification of an incoming message.
   //
   auto layout = TlvLayout();

   if(Dir() == MsgIncoming)
   {
      Debug::SwLog(TlvMessage_AddParm,
         "invalid operation", pack2(pid, layout->header.signal));
      return nullptr;
   }

   //  Check if the fence pattern is trampled.
   //
   CheckFence();

   //  Ensure that the new parameter (and its header) will fit in the
   //  buffer.  The buffer already contains a fence.  Because the new
   //  parameter overwrites it, there will be room for the new fence.
   //
   auto size = sizeof(TlvParmHeader) + Pad(plen) + FenceSize;
   bool moved = false;
   if(!WriteBuffer()->AddBytes(nullptr, size, moved)) return nullptr;

   if(moved)
   {
      Refresh();
      layout = TlvLayout();
   }

   //  The new parameter starts just after the end of the message.  Fill
   //  in its header, update the message's length, and add the fence.
   //
   auto pptr = (TlvParm*) (layout->bytes + layout->header.length);
   pptr->header.pid = pid;
   pptr->header.plen = plen;
   layout->header.length += sizeof(TlvParmHeader) + Pad(plen);
   *FencePtr() = ParmFencePattern;
   return pptr;
}

//------------------------------------------------------------------------------

size_t TlvMessage::AllParms(const TlvParm* ptab[], size_t size) const
{
   Debug::ft("TlvMessage.AllParms");

   size_t count = 0;
   ParmIterator pit;

   for(auto pptr = FirstParm(pit);
      (pptr != nullptr) && (count < size); pptr = NextParm(pit))
   {
      ptab[count++] = pptr;
   }

   return count;
}

//------------------------------------------------------------------------------

void TlvMessage::ChangeDir(MsgDirection nextDir)
{
   Debug::ft("TlvMessage.ChangeDir");

   //  An outgoing message must have a valid parameter fence.
   //
   Message::ChangeDir(nextDir);
   if(nextDir == MsgOutgoing) AddFence();
}

//------------------------------------------------------------------------------

void TlvMessage::CheckFence() const
{
   Debug::ft("TlvMessage.CheckFence");

   //  If the fence has been trampled, kill the context after putting the
   //  death pattern into the buffer to mark the location where trampling
   //  began.
   //
   if(*FencePtr() != ParmFencePattern)
   {
      *FencePtr() = ParmDeathPattern;
      Context::Kill("message trampled", pack2(GetProtocol(), GetSignal()));
   }
}

//------------------------------------------------------------------------------

TlvParm* TlvMessage::CopyParm(const TlvParm& src, ParameterId pid)
{
   Debug::ft("TlvMessage.CopyParm");

   if(pid == NIL_ID) pid = src.header.pid;
   auto pptr = AddParm(pid, src.header.plen);
   if(pptr != nullptr) Memory::Copy(pptr->bytes, src.bytes, src.header.plen);
   return pptr;
}

//------------------------------------------------------------------------------

void TlvMessage::DeleteParm(TlvParm& parm)
{
   Debug::ft("TlvMessage.DeleteParm");

   parm.header.pid = NIL_ID;
}

//------------------------------------------------------------------------------

TlvMessage::Fence* TlvMessage::FencePtr() const
{
   Debug::ft("TlvMessage.FencePtr");

   auto layout = TlvLayout();
   auto fence = layout->bytes + layout->header.length;
   return (Fence*) fence;
}

//------------------------------------------------------------------------------

byte_t* TlvMessage::FindBytes(size_t& size, ParameterId pid) const
{
   Debug::ft("TlvMessage.FindBytes");

   auto pptr = FindParm(pid);

   if(pptr == nullptr)
   {
      size = 0;
      return nullptr;
   }

   size = pptr->header.plen;
   return pptr->bytes;
}

//------------------------------------------------------------------------------

TlvParm* TlvMessage::FindParm(ParameterId pid) const
{
   Debug::ft("TlvMessage.FindParm");

   ParmIterator pit;

   for(auto pptr = FirstParm(pit); pptr != nullptr; pptr = NextParm(pit))
   {
      if(pptr->header.pid == pid) return pptr;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

size_t TlvMessage::FindParms
   (ParameterId pid, const TlvParm* ptab[], size_t size) const
{
   Debug::ft("TlvMessage.FindParms");

   size_t count = 0;
   ParmIterator pit;

   for(auto pptr = FirstParm(pit); pptr != nullptr; pptr = NextParm(pit))
   {
      if((pptr->header.pid == pid) && (count < size))
      {
         ptab[count++] = pptr;
      }
   }

   return count;
}

//------------------------------------------------------------------------------

TlvParm* TlvMessage::FirstParm(ParmIterator& pit) const
{
   Debug::ft("TlvMessage.FirstParm");

   auto layout = TlvLayout();

   pit.mptr = layout;
   pit.pindex = 0;

   if(layout->header.length == 0)
   {
      pit.pptr = nullptr;
      return nullptr;
   }

   pit.pptr = &layout->firstParm;
   if(pit.pptr->header.pid == NIL_ID) return NextParm(pit);
   return pit.pptr;
}

//------------------------------------------------------------------------------

Message::InspectRc TlvMessage::InspectMsg(debug64_t& errval) const
{
   Debug::ft("TlvMessage.InspectMsg");

   auto rc = Message::InspectMsg(errval);
   if(rc != Ok) return rc;

   //e Support message inspection.

   return Ok;
}

//------------------------------------------------------------------------------

bool TlvMessage::MatchParm
   (const TlvParm* pptr, ParmIterator& pit, bool& last) const
{
   Debug::ft("TlvMessage.MatchParm");

   ParmIterator locpit;
   TlvParm* locpptr;

   //  If PPTR is nullptr, don't bother to look for it.
   //
   last = false;
   if(pptr == nullptr) return false;

   //  See if PPTR references one of the parameters.  If it does, advance
   //  to the next parameter and return its iterator in PIT.  If PPTR was
   //  the last parameter, set LAST.
   //
   for(locpptr = FirstParm(locpit);
      (locpptr != pptr) && (locpptr != nullptr); locpptr = NextParm(locpit));

   if(locpptr == pptr)
   {
      if(NextParm(locpit) == nullptr) last = true;
      pit = locpit;
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

TlvParm* TlvMessage::NextParm(ParmIterator& pit) const
{
   Debug::ft("TlvMessage.NextParm");

   size_t nextIndex;

   //  Find the next parameter after PIT.PPTR and update PIT.  If PIT.PPTR
   //  is nullptr or the last parameter, return nullptr without changing PIT.
   //
   if(pit.pptr == nullptr) return nullptr;

   nextIndex = pit.pindex +        // current offset
      sizeof(TlvParmHeader) +      // this parm's header
      Pad(pit.pptr->header.plen);  // this parm's length

   if(nextIndex >= pit.mptr->header.length) return nullptr;
   pit.pindex = nextIndex;
   pit.pptr = (TlvParm*) &pit.mptr->bytes[nextIndex];
   if(pit.pptr->header.pid == NIL_ID) return NextParm(pit);
   return pit.pptr;
}

//------------------------------------------------------------------------------

size_t TlvMessage::ParmOffset(const ParmIterator& pit) const
{
   Debug::ft("TlvMessage.ParmOffset");

   if(pit.mptr == TlvLayout())
   {
      if(pit.pptr != nullptr) return pit.pindex;
   }

   return SIZE_MAX;
}

//------------------------------------------------------------------------------

void TlvMessage::Patch(sel_t selector, void* arguments)
{
   Message::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

bool TlvMessage::Send(Message::Route route)
{
   Debug::ft("TlvMessage.Send");

   //  Check the fence in case trampling occurred after the last AddParm.
   //
   CheckFence();
   return Message::Send(route);
}

//------------------------------------------------------------------------------

fn_name TlvMessage_Wrap = "TlvMessage.Wrap";

TlvParm* TlvMessage::Wrap(const TlvMessage& msg, ParameterId pid)
{
   Debug::ft(TlvMessage_Wrap);

   byte_t* src;

   //  SRCE references MSG's contents.  PLEN is the length of MSG's contents
   //  *plus* its header, which must also be included during encapsulation.
   //
   msg.Payload(src);
   auto plen = sizeof(MsgHeader) + msg.Header()->length;

   if(plen > MaxSbMsgSize)
   {
      Debug::SwLog(TlvMessage_Wrap, "message length", plen);
      return nullptr;
   }

   //  Add a parameter to this message and fill it with SRC.
   //
   auto pptr = AddParm(pid, plen);
   if(pptr == nullptr) return nullptr;
   auto encap = reinterpret_cast<TlvMsgLayout*>(pptr->bytes);
   encap->header = *msg.Header();
   plen -= sizeof(MsgHeader);
   Memory::Copy(encap->bytes, src, plen);
   return pptr;
}
}
