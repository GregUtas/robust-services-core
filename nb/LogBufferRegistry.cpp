//==============================================================================
//
//  LogBufferRegistry.cpp
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
#include "LogBufferRegistry.h"
#include <bitset>
#include <ostream>
#include <utility>
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "LogBuffer.h"
#include "TimePoint.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const size_t LogBufferRegistry::LogBufferSize = 1 * MBs;

//------------------------------------------------------------------------------

LogBufferRegistry::LogBufferRegistry() : size_(0)
{
   Debug::ft("LogBufferRegistry.ctor");
}

//------------------------------------------------------------------------------

fn_name LogBufferRegistry_dtor = "LogBufferRegistry.dtor";

LogBufferRegistry::~LogBufferRegistry()
{
   Debug::ftnt(LogBufferRegistry_dtor);

   Debug::SwLog(LogBufferRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

LogBuffer* LogBufferRegistry::Access(size_t index) const
{
   Debug::ft("LogBufferRegistry.Access");

   if(index >= size_ - 1) return nullptr;
   return buffer_[index].get();
}

//------------------------------------------------------------------------------

LogBuffer* LogBufferRegistry::Active() const
{
   Debug::ftnt("LogBufferRegistry.Active");

   if(size_ == 0) return nullptr;
   return buffer_[size_ - 1].get();
}

//------------------------------------------------------------------------------

void LogBufferRegistry::Compress()
{
   Debug::ft("LogBufferRegistry.Compress");

   for(size_t i = 0; i < size_ - 1; ++i)
   {
      if(buffer_[i] != nullptr) continue;

      for(size_t j = i + 1; j < size_; ++j)
      {
         if(buffer_[j] != nullptr)
         {
            buffer_[i] = std::move(buffer_[j]);
            break;
         }
      }
   }

   for(size_t i = 0; i < MaxBuffers; ++i)
   {
      if(buffer_[i] == nullptr)
      {
         size_ = i;
         return;
      }
   }

   size_ = MaxBuffers;
}

//------------------------------------------------------------------------------

void LogBufferRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   auto lead1 = prefix + spaces(2);
   auto lead2 = prefix + spaces(4);

   stream << prefix << "buffers [index]" << CRLF;

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
   if(size_ == 0) return "logs" + TimePoint::TimeZeroStr() + ".txt";
   return buffer_[size_ - 1]->FileName();
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
      Debug::SwLog(LogBufferRegistry_Free, "invalid index", index);
      return false;
   }

   buffer_[index].reset();
   Compress();
   return true;
}

//------------------------------------------------------------------------------

void LogBufferRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void LogBufferRegistry::Startup(RestartLevel level)
{
   Debug::ft("LogBufferRegistry.Startup");

   //  Allocate a log buffer during each restart.  When the system is booting,
   //  don't unprotect immutable memory, because it needs to stay unprotected
   //  during initialization.
   //
   FunctionGuard guard(Guard_ImmUnprotect);

   buffer_[size_] = std::unique_ptr< LogBuffer >(new LogBuffer(LogBufferSize));
   ++size_;

   //  Delete all empty buffers except the new one.
   //
   for(size_t i = 0; i < size_ - 1; ++i)
   {
      if(buffer_[i]->Empty())
      {
         buffer_[i].reset();
      }
   }

   Compress();

   //  If the array of buffers is full, delete the oldest buffer in case
   //  another restart occurs.
   //
   if(size_ == MaxBuffers)
   {
      Free(0);
   }
}
}
