//==============================================================================
//
//  TlvMessage.h
//
//  Copyright (C) 2013-2020  Greg Utas
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
#ifndef TLVMESSAGE_H_INCLUDED
#define TLVMESSAGE_H_INCLUDED

#include "Message.h"
#include <cstddef>
#include <cstdint>
#include "Debug.h"
#include "Memory.h"
#include "MsgHeader.h"
#include "Parameter.h"
#include "SbIpBuffer.h"
#include "SbTypes.h"
#include "SysTypes.h"
#include "TlvParameter.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Supports messages whose parameters are encoded in TLV format.  Although
//  this class can be used directly, any non-trivial protocol should usually
//  define its own subclass or per-signal subclasses.
//
class TlvMessage : public Message
{
public:
   //  Overridden to create an incoming message.
   //
   explicit TlvMessage(SbIpBufferPtr& buff);

   //  Overridden to create an outgoing message.
   //
   TlvMessage(ProtocolSM* psm, size_t size);

   //  Supports message decapsulation.  PARM is an encapsulated message
   //  that was created using WRAP (see below).  It has now arrived at its
   //  destination, which wants to unwrap it to create an incoming message.
   //  PARM also contains the message header, which is placed into the new
   //  message's header.
   //
   TlvMessage(const TlvParmLayout& parm, ProtocolSM* psm);

   //  Copies MSG into an outgoing message and queues it on PSM.  The header
   //  contains the message length but is not changed in any other way.
   //
   TlvMessage(const Message& msg, ProtocolSM* psm);

   //  Virtual to allow subclassing.
   //
   virtual ~TlvMessage();

   //  Encapsulates MSG's payload as a parameter within the message, giving
   //  it the identifier PID.
   //
   virtual TlvParmPtr Wrap(const TlvMessage& msg, ParameterId pid);

   //  Returns the first parameter that matches PID.  Returns nullptr if no
   //  such parameter exists.  T is the type for the parameter's contents,
   //  omitting the TLV header.  The syntax for invocation on MSG is
   //    auto info = msg.FindType< T >(pid);
   //
   template< class T > T* FindType(ParameterId pid) const
   {
      NodeBase::Debug::ft(TlvMessage_FindType());
      auto pptr = FindParm(pid);
      if(pptr == nullptr) return nullptr;
      return reinterpret_cast< T* >(pptr->bytes);
   }

   //  Adds a parameter of type T (PARM) that is identified by PID.
   //  The syntax for invocation on MSG is
   //    auto info = msg.AddType(parm, pid);
   //  where PARM is of type T, with its fields already filled in
   //  (although they can also be filled in afterwards).
   //
   template< class T > T* AddType(const T& parm, ParameterId pid)
   {
      NodeBase::Debug::ft(TlvMessage_AddType());
      auto pptr = AddParm(pid, sizeof(T));
      if(pptr == nullptr) return nullptr;
      auto dest = reinterpret_cast< T* >(pptr->bytes);
      *dest = parm;
      return dest;
   }

   //  Searches icMsg for a parameter of type T, identified by icPid.  If
   //  one is found, it is copied into this message using the identifier
   //  ogPid.  If ogPid is not provided, icPid is also used for the copy.
   //  The syntax for invocation on MSG is
   //    auto info = msg.CopyType< T >(icMsg, icPid, ogPid);
   //
   template< class T > T* CopyType
      (const TlvMessage& icMsg, ParameterId icPid, ParameterId ogPid = 0)
   {
      NodeBase::Debug::ft(TlvMessage_CopyType());
      if(ogPid == NodeBase::NIL_ID) ogPid = icPid;
      auto pptr = icMsg.FindType< T >(icPid);
      if(pptr != nullptr) return AddType(*pptr, ogPid);
      return nullptr;
   }

   //  Looks for a parameter of type T, identified by PID, and updates
   //  PARM accordingly.  Returns PID if
   //  o the parameter is found and USE is Illegal, or
   //  o the parameter is not found and USE is Mandatory.
   //  Returns 0 otherwise.  The syntax for invocation on MSG is
   //    T* parm;
   //    auto rc = msg.VerifyParm(pid, use, parm);
   //
   template< class T > Parameter::TestRc VerifyParm
      (ParameterId pid, Parameter::Usage use, T*& parm) const
   {
      NodeBase::Debug::ft(TlvMessage_VerifyParm());

      auto pptr = FindParm(pid);
      parm = (pptr == nullptr ? nullptr : reinterpret_cast< T* >(pptr->bytes));

      if((pptr == nullptr) && (use == Parameter::Mandatory))
         return Parameter::MessageMissingMandatoryParm;
      if((pptr != nullptr) && (use == Parameter::Illegal))
         return Parameter::MessageContainsIllegalParm;
      return Parameter::Ok;
   }
protected:
   //  A forward declaration.
   //
   struct TlvMsgLayout;
public:
   //  For iterating through a TLV message's parameters.
   //
   class ParmIterator
   {
      friend class TlvMessage;
   public:
      ParmIterator() : mptr(nullptr), pptr(nullptr), pindex(0) { }
   private:
      const TlvMsgLayout* mptr;  // reference to message
      TlvParmPtr pptr;           // reference to current parameter
      size_t pindex;             // parameter's offset within message
   };

   //  Returns the first parameter that matches PID.  Returns nullptr if no
   //  such parameter exists.
   //
   TlvParmPtr FindParm(ParameterId pid) const;

   //  Returns the first parameter in the message and updates PIT, which
   //  is used to iterate through the parameters.
   //
   TlvParmPtr FirstParm(ParmIterator& pit) const;

   //  Returns the next parameter in the message based on PIT, which is
   //  updated.
   //
   TlvParmPtr NextParm(ParmIterator& pit) const;

   //  Returns all parameters in the message by updating PTAB, an array
   //  that contains SIZE elements (indices 0 to SIZE-1).  Returns the
   //  number of parameters found.
   //
   size_t AllParms(TlvParmArray ptab, size_t size) const;

   //  Returns all parameters that match PID by updating PTAB, an array that
   //  contains SIZE elements (indices 0 to SIZE-1).  Returns the number of
   //  parameters found.
   //
   size_t FindParms(ParameterId pid, TlvParmArray ptab, size_t size) const;

   //  Adds a parameter to the message.  PID is its identifier and PLEN
   //  is its length in bytes.  The syntax for invocation on MSG, where
   //  T is the type for the parameter's contents, is
   //    auto pptr = msg.AddParm(pid, plen);
   //    auto info = reinterpret_cast< T* >(pptr->bytes);
   //  after which INFO's fields can be filled in.
   //
   virtual TlvParmPtr AddParm(ParameterId pid, size_t plen);

   //  Inserts a parameter identified by PID, filling it with SIZE bytes
   //  that are taken from SRC.
   //
   TlvParmPtr AddBytes
      (const NodeBase::byte_t* src, size_t size, ParameterId pid);

   //  Copies the parameter SRC (in another message) into this message by
   //  creating a parameter identified by PID.  If PID is NIL_ID, SRCE's
   //  parameter identifier is used.
   //
   TlvParmPtr CopyParm
      (const TlvParmLayout& src, ParameterId pid = NodeBase::NIL_ID);

   //  Removes a parameter by changing its identifier to NIL_ID.
   //
   virtual void DeleteParm(TlvParmLayout& parm);

   //  Overridden to inspect the message's contents.
   //
   InspectRc InspectMsg(NodeBase::debug64_t& errval) const override;

   //  Overridden to check the fence pattern before sending the message.
   //
   bool Send(Message::Route route) override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //> The byte alignment used for messages in this network.  The default
   //  value pads the header and parameters to a multiple of four bytes.
   //
   static const size_t Log2Align = 2;

   //  Given a structure of SIZE bytes, this returns the value that rounds
   //  SIZE up to a multiple of 2^Log2Align ('^' meaning exponentiation).
   //
   static size_t Pad(size_t size)
   {
      return NodeBase::Memory::Align(size, Log2Align);
   }
protected:
   //  The physical layout of a TLV message's data.
   //
   struct TlvMsgLayout
   {
      MsgHeader header;
      union
      {
         TlvParmLayout firstParm;                   // first parameter
         NodeBase::byte_t bytes[MaxSbMsgSize - 1];  // payload as bytes
      };
   };

   //  The type for the fence that is placed after a parameter to detect
   //  trampling.
   //
   typedef uint16_t Fence;
   static const size_t FenceSize = sizeof(Fence);

   //  Finds a byte array that is identified by PID, and returns its
   //  length in SIZE.
   //
   NodeBase::byte_t* FindBytes(size_t& size, ParameterId pid) const;

   //  Returns true if PPTR references a parameter within this message, in
   //  which case PIT is updated to reference the parameter that *follows*
   //  PPTR.  LAST is set to false unless PPTR is the last parameter in the
   //  message.
   //
   virtual bool MatchParm(TlvParmPtr pptr, ParmIterator& pit, bool& last) const;

   //  Adds the fence pattern to an incoming message in preparation for
   //  adding more parameters to it.
   //
   virtual void AddFence();

   //  Returns the entire TLV message (header plus parameters).
   //
   TlvMsgLayout* TlvLayout() const
      { return reinterpret_cast< TlvMsgLayout* >(Buffer()->HeaderPtr()); }

   //  Returns the number of bytes that precede the parameter referenced by
   //  PPTR.  Returns SIZE_MAX if PPTR is nullptr or not within this message.
   //
   size_t ParmOffset(ParmIterator& pit) const;

   //  Returns a pointer to the message's fence, which follows the header
   //  and parameters in TlvLayout.
   //
   Fence* FencePtr() const;

   //  Kills the running context if the message fence has been overwritten.
   //
   void CheckFence() const;

   //  This marker is placed after a parameter when it is added to a message.
   //  o The fence is not included in MsgHeader.length.
   //  o An incoming message does not contain a fence.
   //  o When a parameter is added, enough bytes are obtained to append
   //    the fence.
   //
   static const Fence ParmFencePattern = 0xaaaa;

   //  This marker is placed after a parameter when the one above was trampled.
   //
   static const Fence ParmDeathPattern = 0xdead;
private:
   //  Overridden to change the message's direction.
   //
   void ChangeDir(NodeBase::MsgDirection nextDir) override;

   //  See the comment in Singleton.h about an fn_name in a template header.
   //
   inline static NodeBase::fn_name TlvMessage_FindType()
      { return "TlvMessage.FindType"; }
   inline static NodeBase::fn_name TlvMessage_AddType()
      { return "TlvMessage.AddType"; }
   inline static NodeBase::fn_name TlvMessage_CopyType()
      { return "TlvMessage.CopyType"; }
   inline static NodeBase::fn_name TlvMessage_VerifyParm()
      { return "TlvMessage.VerifyParm"; }
};
}
#endif
