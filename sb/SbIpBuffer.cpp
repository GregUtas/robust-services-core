//==============================================================================
//
//  SbIpBuffer.cpp
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
#include "SbIpBuffer.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "MsgHeader.h"
#include "Protocol.h"
#include "ProtocolRegistry.h"
#include "SbCliParms.h"
#include "SbPools.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name SbIpBuffer_ctor1 = "SbIpBuffer.ctor";

SbIpBuffer::SbIpBuffer(MsgDirection dir, size_t payload) :
   IpBuffer(dir, sizeof(MsgHeader), payload)
{
   Debug::ft(SbIpBuffer_ctor1);

   auto header = Header();
   if(header != nullptr) *header = MsgHeader();
}

//------------------------------------------------------------------------------

fn_name SbIpBuffer_ctor2 = "SbIpBuffer.ctor(copy)";

SbIpBuffer::SbIpBuffer(const SbIpBuffer& that) : IpBuffer(that)
{
   Debug::ft(SbIpBuffer_ctor2);
}

//------------------------------------------------------------------------------

fn_name SbIpBuffer_dtor = "SbIpBuffer.dtor";

SbIpBuffer::~SbIpBuffer()
{
   Debug::ft(SbIpBuffer_dtor);
}

//------------------------------------------------------------------------------

void SbIpBuffer::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   IpBuffer::Display(stream, prefix, options);

   auto header = Header();

   stream << prefix << "MsgHeader:" << CRLF;
   header->Display(stream, prefix + spaces(2));
   stream << prefix << "Parameters:" << CRLF;

   auto reg = Singleton< ProtocolRegistry >::Instance();
   auto pro = reg->GetProtocol(header->protocol);

   if(pro != nullptr)
      pro->DisplayMsg(stream, prefix + spaces(2), *this);
   else
      stream << prefix << spaces(2) << NoProtocolExpl << CRLF;
}

//------------------------------------------------------------------------------

fn_name SbIpBuffer_delete1 = "SbIpBuffer.operator delete";

void SbIpBuffer::operator delete(void* addr)
{
   Debug::ft(SbIpBuffer_delete1);

   Pooled::operator delete(addr);
}

//------------------------------------------------------------------------------

fn_name SbIpBuffer_delete2 = "SbIpBuffer.operator delete(user)";

void SbIpBuffer::operator delete(void* addr, SbPoolUser user)
{
   Debug::ft(SbIpBuffer_delete2);

   Pooled::operator delete(addr);
}

//------------------------------------------------------------------------------

fn_name SbIpBuffer_new = "SbIpBuffer.operator new";

void* SbIpBuffer::operator new(size_t size, SbPoolUser user)
{
   Debug::ft(SbIpBuffer_new);

   switch(user)
   {
   case PayloadUser:
      return Singleton< SbIpBufferPool >::Instance()->DeqBlock(size);
   case ToolUser:
      return Singleton< BtIpBufferPool >::Instance()->DeqBlock(size);
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void SbIpBuffer::Patch(sel_t selector, void* arguments)
{
   IpBuffer::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name SbIpBuffer_PayloadSize = "SbIpBuffer.PayloadSize";

size_t SbIpBuffer::PayloadSize() const
{
   Debug::ft(SbIpBuffer_PayloadSize);

   return Header()->length;
}
}
