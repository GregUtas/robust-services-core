//==============================================================================
//
//  SbIpBuffer.cpp
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

using namespace NodeBase;
using namespace NetworkBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
SbIpBuffer::SbIpBuffer(MsgDirection dir, size_t payload) :
   IpBuffer(dir, sizeof(MsgHeader), payload)
{
   Debug::ft("SbIpBuffer.ctor");

   auto header = Header();
   if(header != nullptr) *header = MsgHeader();
}

//------------------------------------------------------------------------------

SbIpBuffer::~SbIpBuffer()
{
   Debug::ftnt("SbIpBuffer.dtor");
}

//------------------------------------------------------------------------------

SbIpBuffer::SbIpBuffer(const SbIpBuffer& that) : IpBuffer(that)
{
   Debug::ft("SbIpBuffer.ctor(copy)");
}

//------------------------------------------------------------------------------

IpBuffer* SbIpBuffer::Clone() const
{
   Debug::ft("SbIpBuffer.Clone");

   return new SbIpBuffer(*this);
}

//------------------------------------------------------------------------------

void SbIpBuffer::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   IpBuffer::Display(stream, prefix, options);

   auto header = Header();

   stream << prefix << "MsgHeader (length=" << sizeof(MsgHeader) << ')' << CRLF;
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

void SbIpBuffer::operator delete(void* addr)
{
   Debug::ftnt("SbIpBuffer.operator delete");

   Pooled::operator delete(addr);
}

//------------------------------------------------------------------------------

void SbIpBuffer::operator delete(void* addr, SbPoolUser user)
{
   Debug::ftnt("SbIpBuffer.operator delete(user)");

   Pooled::operator delete(addr);
}

//------------------------------------------------------------------------------

void* SbIpBuffer::operator new(size_t size, SbPoolUser user)
{
   Debug::ft("SbIpBuffer.operator new");

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

size_t SbIpBuffer::PayloadSize() const
{
   Debug::ft("SbIpBuffer.PayloadSize");

   return Header()->length;
}
}
