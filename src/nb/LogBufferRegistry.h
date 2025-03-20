//==============================================================================
//
//  LogBufferRegistry.h
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
#ifndef LOGBUFFERREGISTRY_H_INCLUDED
#define LOGBUFFERREGISTRY_H_INCLUDED

#include "Immutable.h"
#include <cstddef>
#include <memory>
#include <string>
#include "NbTypes.h"
#include "SysTypes.h"

namespace NodeBase
{
   class LogBuffer;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for log buffers.
//
class LogBufferRegistry : public Immutable
{
   friend class Singleton<LogBufferRegistry>;
public:
   //  Deleted to prohibit copying.
   //
   LogBufferRegistry(const LogBufferRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   LogBufferRegistry& operator=(const LogBufferRegistry& that) = delete;

   //> The maximum number of log buffers.
   //
   static const id_t MaxBuffers = 8;

   //  Returns the name of the log file.
   //
   std::string FileName() const;

   //  Returns the active log buffer.
   //
   LogBuffer* Active() const;

   //  Returns buffer_[INDEX].  Returns nullptr if INDEX is invalid
   //  or that of the active buffer.
   //
   LogBuffer* Access(size_t index) const;

   //  Deletes buffer_[INDEX].  Returns false if INDEX is invalid or
   //  that of the active buffer.  A FunctionGuard must be used to
   //  unprotect MemImmutable before invoking this function.
   //
   bool Free(size_t index);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;
private:
   //  Private because this is a singleton.
   //
   LogBufferRegistry();

   //  Private because this is a singleton.
   //
   ~LogBufferRegistry();

   //  Keeps the buffers contiguous after deleting one or more buffers.
   //
   void Compress();

   //  The number of buffers currently allocated.
   //
   size_t size_;

   //  The buffers.
   //
   std::unique_ptr<LogBuffer> buffer_[MaxBuffers];
};
}
#endif
