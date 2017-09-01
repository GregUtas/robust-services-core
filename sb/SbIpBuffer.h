//==============================================================================
//
//  SbIpBuffer.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SBIPBUFFER_H_INCLUDED
#define SBIPBUFFER_H_INCLUDED

#include "IpBuffer.h"
#include <cstddef>
#include "NbTypes.h"
#include "SbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  SbIpBuffer wraps a message that passes between SessionBase and the IP
//  stack.  Its base class allocates a byte buffer that holds a contiguous
//  message with a SessionBase header.
//
//  This class is not intended to be subclassed.  Its use is restricted to
//  input handlers and to Message and its subclasses.
//
class SbIpBuffer : public IpBuffer
{
public:
   //  Allocates a buffer that can accommodate a MsgHeader and PAYLOAD.
   //  DIR specifies whether the buffer will receive or send a message.
   //  The MsgHeader is initialized, but the user of this interface is
   //  responsible for updating its contents (including the length).
   //
   SbIpBuffer(MsgDirection dir, MsgSize payload);

   //  Copy constructor.
   //
   SbIpBuffer(const SbIpBuffer& that);

   //  Not subclassed.
   //
   ~SbIpBuffer();

   //  Returns a pointer to the SessionBase message header.
   //
   MsgHeader* Header() const
      { return reinterpret_cast< MsgHeader* >(HeaderPtr()); }

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;

   //  Obtains a buffer from the object pool used by USER.
   //
   static void* operator new(size_t size, SbPoolUser user = PayloadUser);

   //  Overridden to invoke the base class operator delete.
   //
   static void operator delete(void* addr);

   //  Returns a buffer to its object pool after the constructor trapped.
   //
   static void operator delete(void* addr, SbPoolUser user);
protected:
   //  Overridden to return the size of Header()->length.
   //
   virtual MsgSize PayloadSize() const override;
};
}
#endif
