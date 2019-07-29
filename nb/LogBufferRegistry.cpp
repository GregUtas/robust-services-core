//==============================================================================
//
//  LogBufferRegistry.cpp
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
#include "LogBufferRegistry.h"
#include <bitset>
#include <cstdint>
#include <ostream>
#include <utility>
#include "Clock.h"
#include "Debug.h"
#include "Formatters.h"
#include "LogBuffer.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name LogBufferRegistry_ctor = "LogBufferRegistry.ctor";

LogBufferRegistry::LogBufferRegistry() : size_(0)
{
   Debug::ft(LogBufferRegistry_ctor);
}

//------------------------------------------------------------------------------

fn_name LogBufferRegistry_dtor = "LogBufferRegistry.dtor";

LogBufferRegistry::~LogBufferRegistry()
{
   Debug::ft(LogBufferRegistry_dtor);
}

//------------------------------------------------------------------------------

fn_name LogBufferRegistry_Access = "LogBufferRegistry.Access";

LogBuffer* LogBufferRegistry::Access(size_t index) const
{
   Debug::ft(LogBufferRegistry_Access);

   if(index >= size_ - 1) return nullptr;
   return buffer_[index].get();
}

//------------------------------------------------------------------------------

void LogBufferRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto lead1 = prefix + spaces(2);
   auto lead2 = prefix + spaces(4);

   stream << prefix << "Buffers [index]:" << CRLF;

   for(size_t i = 0; i < size_; ++i)
   {
      stream << lead1 << strIndex(i);
      stream << (options.test(DispVerbose) ? CRLF : SPACE);
      buffer_[i]->Display(stream, lead2, options);
   }
}

//------------------------------------------------------------------------------

string LogBufferRegistry::FileName() const
{
   if(size_ == 0) return "logs" + Clock::TimeZeroStr() + ".txt";
   return buffer_[size_ - 1]->FileName();
}

//------------------------------------------------------------------------------

size_t LogBufferRegistry::Find(const LogBuffer* buff) const
{
   if((size_ == 0) || (buff == nullptr)) return SIZE_MAX;

   for(size_t i = 0; i < size_; ++i)
   {
      if(buffer_[i].get() == buff) return i;
   }

   return SIZE_MAX;
}

//------------------------------------------------------------------------------

fn_name LogBufferRegistry_First = "LogBufferRegistry.First";

LogBuffer* LogBufferRegistry::First() const
{
   Debug::ft(LogBufferRegistry_First);

   if(size_ == 0) return nullptr;
   return buffer_[size_ - 1].get();
}

//------------------------------------------------------------------------------

fn_name LogBufferRegistry_Free = "LogBufferRegistry.Free";

bool LogBufferRegistry::Free(size_t index)
{
   Debug::ft(LogBufferRegistry_Free);

   //  Check that INDEX is in range and is not the active buffer.
   //
   if(index >= size_ - 1)
   {
      Debug::SwLog(LogBufferRegistry_Free, index, 0);
      return false;
   }

   buffer_[index].reset();
   if(--size_ == 0) return true;

   //  Keep the buffers contiguous.
   //
   while(index < size_)
   {
      buffer_[index] = std::move(buffer_[index + 1]);
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name LogBufferRegistry_Last = "LogBufferRegistry.Last";

LogBuffer* LogBufferRegistry::Last() const
{
   Debug::ft(LogBufferRegistry_Last);

   if(size_ == 0) return nullptr;
   return buffer_[0].get();
}

//------------------------------------------------------------------------------

void LogBufferRegistry::Next(LogBuffer*& buff) const
{
   auto i = Find(buff);

   if((i >= size_ - 1) || (i == SIZE_MAX))
   {
      buff = nullptr;
      return;
   }

   buff = buffer_[i + 1].get();
}

//------------------------------------------------------------------------------

void LogBufferRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void LogBufferRegistry::Prev(LogBuffer*& buff) const
{
   auto i = Find(buff);

   if((i == 0) || (i == SIZE_MAX))
   {
      buff = nullptr;
      return;
   }

   buff = buffer_[i - 1].get();
}

//------------------------------------------------------------------------------

fn_name LogBufferRegistry_Startup = "LogBufferRegistry.Startup";

void LogBufferRegistry::Startup(RestartLevel level)
{
   Debug::ft(LogBufferRegistry_Startup);

   //  Delete all empty buffers.
   //
   for(size_t i = 0; i < size_; ++i)
   {
      if(buffer_[i]->First() == nullptr)
      {
         Free(i);
      }
   }

   //  If the array of buffers is full, delete the second-oldest one.  This
   //  preserves the oldest buffer in case a restart loop occurs.
   //
   if(size_ == MaxBuffers)
   {
      Free(1);
   }

   //  Allocate a log buffer during each restart.
   //
   buffer_[size_] = std::unique_ptr< LogBuffer >(new LogBuffer(LogBufferSize));
   ++size_;
}
}
