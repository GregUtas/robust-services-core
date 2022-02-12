//==============================================================================
//
//  SbIpBuffer.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
#ifndef SBIPBUFFER_H_INCLUDED
#define SBIPBUFFER_H_INCLUDED

#include "IpBuffer.h"
#include <cstddef>
#include "NbTypes.h"
#include "SbTypes.h"

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
class SbIpBuffer : public NetworkBase::IpBuffer
{
public:
   //  Allocates a buffer that can accommodate a MsgHeader and PAYLOAD.
   //  DIR specifies whether the buffer will receive or send a message.
   //  The MsgHeader is initialized, but the user of this interface is
   //  responsible for updating its contents (including the length).
   //
   SbIpBuffer(NodeBase::MsgDirection dir, size_t payload);

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

   //  Obtains a buffer from the object pool used by USER.
   //
   static void* operator new(size_t size, SbPoolUser user = PayloadUser);

   //  Overridden to invoke the base class operator delete.
   //
   static void operator delete(void* addr);

   //  Returns a buffer to its object pool after the constructor trapped.
   //
   static void operator delete(void* addr, SbPoolUser user);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to return the size of Header()->length.
   //
   size_t PayloadSize() const override;
};
}
#endif
