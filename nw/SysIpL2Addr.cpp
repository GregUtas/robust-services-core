//==============================================================================
//
//  SysIpL2Addr.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "SysIpL2Addr.h"
#include <cctype>
#include <cstddef>
#include <ios>
#include <iosfwd>
#include <sstream>
#include "Debug.h"
#include "Formatters.h"
#include "SysSocket.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Parses the decimal byte that follows TEXT[INDEX] and returns it in BYTE.
//  Returns FALSE if a decimal byte does not follow TEXT[INDEX] or its value
//  is out of range.
//
static bool GetDecByte(const string& text, size_t& index, uint8_t& byte)
{
   Debug::ft("NetworkBase.GetDecByte");

   auto found = false;
   uword value = 0;

   index = strSkipSpaces(text, index);
   byte = 0;

   while(index < text.size())
   {
      if(!isdigit(text[index])) break;

      found = true;
      value = (value * 10) + (text[index++] - '0');
      if(value > 255) return false;
   }

   byte = uint8_t(value);
   return found;
}

//------------------------------------------------------------------------------
//
//  Parses the hex quartet that starts at TEXT[INDEX] and returns it in BYTE.
//  Returns FALSE if a hex quartet does not start at TEXT[INDEX] or its value
//  is out of range.
//
static bool GetHexQuartet(const string& text, size_t& index, uint16_t& quartet)
{
   Debug::ft("NetworkBase.GetHexQuartet");

   auto count = 0;
   uword value = 0;
   uint8_t n = 0;

   index = strSkipSpaces(text, index);
   quartet = 0;

   while(index < text.size())
   {
      if(!isxdigit(text[index])) break;

      if(++count > 4) return false;

      if(text[index] < 'A')
         n = text[index++] - '0';
      else if(text[index] < 'a')
         n = text[index++] - 'A' + 10;
      else
         n = text[index++] - 'a' + 10;

      value = (value << 4) + n;
   }

   quartet = uint16_t(value);
   return (count > 0);
}

//==============================================================================
//
//  The null IP address.
//
static const SysIpL2Addr NullIpAddr_;

//  The IPv4 loopback address.
//
static const SysIpL2Addr LoopbackIPv4Addr_("127.0.0.1");

//  The IPv6 loopback address.
//
static const SysIpL2Addr LoopbackIPv6Addr_("0:0:0:0:0:0:0:1");

//------------------------------------------------------------------------------

SysIpL2Addr::SysIpL2Addr()
{
   Debug::ftnt("SysIpL2Addr.ctor");
}

//------------------------------------------------------------------------------

SysIpL2Addr::SysIpL2Addr(IPv4Addr netaddr)
{
   Debug::ft("SysIpL2Addr.ctor(IPv4)");

   addr_.u32[U32_IPv4_IDX] = ntohl(netaddr);
   addr_.SetAsMappedIPv4Addr();
}

//------------------------------------------------------------------------------

SysIpL2Addr::SysIpL2Addr(const uint16_t netaddr[8])
{
   Debug::ft("SysIpL2Addr.ctor(IPv6)");

   for(int i = 7; i >= 0; --i)
   {
      addr_.u16[i] = ntohs(netaddr[i]);
   }
}

//------------------------------------------------------------------------------

SysIpL2Addr::SysIpL2Addr(const string& text)
{
   Debug::ft("SysIpL2Addr.ctor(string)");

   Nullify();

   auto valid = false;
   auto index = strSkipSpaces(text, 0);

   if(text.find('.') != string::npos)
   {
      uint8_t byte;

      //  This should be an IPv4 address.  Extract its four bytes, which
      //  must be separated by periods.  This must get us to the end of
      //  TEXT unless a port number (introduced by a colon) follows.
      //
      for(size_t n = 1; n <= 4; ++n)
      {
         if(!GetDecByte(text, index, byte)) break;
         index = strSkipSpaces(text, index);

         addr_.u32[U32_IPv4_IDX] = (addr_.u32[U32_IPv4_IDX] << 8) + byte;

         if(n == 4)
         {
            if((index >= text.size()) || (text[index] == ':'))
            {
               valid = true;
               addr_.SetAsMappedIPv4Addr();
            }
         }
         else if(index < text.size())
         {
            if(text[index++] != '.') break;
         }
      }
   }
   else if(text.find(':') != string::npos)
   {
      //  This should be an IPv6 address.  Extract its eight quartets, which
      //  must be separated by colons.  This must get us to the end of TEXT
      //  unless a port number (preceded by "]:") follows.  A double colon
      //  is not supported.
      //
      uint16_t quartet;
      auto bracket = (text[index] == '[');
      if(bracket) index = strSkipSpaces(text, index + 1);

      for(size_t n = 0; n < 8; ++n)
      {
         if(!GetHexQuartet(text, index, quartet)) break;
         index = strSkipSpaces(text, index);

         if(n == 7)
         {
            if((index >= text.size()) || (bracket && (text[index] == ']')))
            {
               addr_.u16[7] = quartet;
               valid = true;
            }
         }
         else if(index < text.size())
         {
            if(text[index++] != ':') break;
            addr_.u16[n] = quartet;
         }
      }
   }

   if(!valid)
   {
      Nullify();
   }
}

//------------------------------------------------------------------------------

SysIpL2Addr::~SysIpL2Addr()
{
   Debug::ftnt("SysIpL2Addr.dtor");
}

//------------------------------------------------------------------------------

void SysIpL2Addr::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Object::Display(stream, prefix, options);

   stream << prefix << "addr : " << to_str() << CRLF;
}

//------------------------------------------------------------------------------

IpAddrFamily SysIpL2Addr::Family() const
{
   Debug::ft("SysIpL2Addr.Family");

   //  If IPv6 is not supported, the non-IPv4 part of an address is zeroed.
   //  A non-zero value in that portion therefore denotes an IPv6 address.
   //
   if(!SupportsIPv6())
   {
      for(int i = U16_MAPPED_IPv4_IDX - 1; i >= 0; --i)
      {
         if(addr_.u16[i] != 0) return IPv6;
      }

      return IPv4;
   }

   //  This is an IPv6 address unless it begins with 0:0:0:0:0:ffff, in which
   //  case it is an IPv4-mapped address.
   //
   if(addr_.u16[U16_MAPPED_IPv4_IDX] != MappedIPv4Quartet) return IPv6;

   for(int i = U16_MAPPED_IPv4_IDX - 1; i >= 0; --i)
   {
      if(addr_.u16[i] != 0) return IPv6;
   }

   return IPv4;
}

//------------------------------------------------------------------------------

void SysIpL2Addr::HostToNetwork(IPv4Addr& netaddr) const
{
   Debug::ft("SysIpL2Addr.HostToNetwork(IPv4)");

   netaddr = htonl(addr_.u32[U32_IPv4_IDX]);
}

//------------------------------------------------------------------------------

void SysIpL2Addr::HostToNetwork(uint16_t netaddr[8]) const
{
   Debug::ft("SysIpL2Addr.HostToNetwork(IPv6)");

   for(size_t i = 0; i < 8; ++i)
   {
      netaddr[i] = htons(addr_.u16[i]);
   }
}

//------------------------------------------------------------------------------

bool SysIpL2Addr::IsLoopbackIpAddr() const
{
   Debug::ft("SysIpL2Addr.IsLoopbackIpAddr");

   if(*this == LoopbackIPv6Addr_) return true;
   if(Family() == IPv6) return false;
   return (addr_.u8[U8_IPv4_A_IDX] == 127);
}

//------------------------------------------------------------------------------

bool SysIpL2Addr::IsValid() const
{
   Debug::ft("SysIpL2Addr.IsValid");

   return (*this != NullIpAddr_);
}

//------------------------------------------------------------------------------

const SysIpL2Addr& SysIpL2Addr::LoopbackIpAddr()
{
   Debug::ft("SysIpL2Addr.LoopbackIpAddr");

   return (SupportsIPv6() ? LoopbackIPv6Addr_ : LoopbackIPv4Addr_);
}

//------------------------------------------------------------------------------

void SysIpL2Addr::NetworkToHost(IPv4Addr netaddr)
{
   Debug::ft("SysIpL2Addr.NetworkToHost(IPv4)");

   addr_.u32[U32_IPv4_IDX] = ntohl(netaddr);
   addr_.SetAsMappedIPv4Addr();
}

//------------------------------------------------------------------------------

void SysIpL2Addr::NetworkToHost(const uint16_t netaddr[8])
{
   Debug::ft("SysIpL2Addr.NetworkToHost(IPv6)");

   for(int i = 7; i >= 0; --i)
   {
      addr_.u16[i] = ntohs(netaddr[i]);
   }
}

//------------------------------------------------------------------------------

void SysIpL2Addr::Nullify()
{
   Debug::ft("SysIpL2Addr.Nullify");

   addr_ = IPv6Addr();
}

//------------------------------------------------------------------------------

const SysIpL2Addr& SysIpL2Addr::NullIpAddr()
{
   return NullIpAddr_;
}

//------------------------------------------------------------------------------

bool SysIpL2Addr::operator==(const SysIpL2Addr& that) const
{
   return (this->addr_ == that.addr_);
}

//------------------------------------------------------------------------------

bool SysIpL2Addr::operator!=(const SysIpL2Addr& that) const
{
   return !(this->addr_ == that.addr_);
}

//------------------------------------------------------------------------------

string SysIpL2Addr::to_str() const
{
   std::ostringstream stream;

   if(Family() == IPv4)
   {
      if(SupportsIPv6())
      {
         //  This should display the 0:0:0:0:0:f: prefix for the IPv4 address.
         //
         stream << std::hex;

         for(size_t i = 0; i <= U16_MAPPED_IPv4_IDX; ++i)
         {
            stream << addr_.u16[i];
            stream << ':';
         }

         stream << std::dec;
      }

      stream << int(addr_.u8[U8_IPv4_A_IDX]) << '.';
      stream << int(addr_.u8[U8_IPv4_B_IDX]) << '.';
      stream << int(addr_.u8[U8_IPv4_C_IDX]) << '.';
      stream << int(addr_.u8[U8_IPv4_D_IDX]);
   }
   else
   {
      stream << std::hex;

      for(size_t i = 0; i < 8; ++i)
      {
         stream << addr_.u16[i];
         if(i != 7) stream << ':';
      }
   }

   return stream.str();
}
}
