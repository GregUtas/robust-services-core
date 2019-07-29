//==============================================================================
//
//  LogThread.h
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
#ifndef LOGTHREAD_H_INCLUDED
#define LOGTHREAD_H_INCLUDED

#include "Thread.h"
#include <cstddef>
#include "NbTypes.h"
#include "SysMutex.h"
#include "SysTypes.h"

namespace NodeBase
{
   class LogBuffer;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Thread for spooling logs.
//
class LogThread : public Thread
{
   friend class Singleton< LogThread >;
   friend class Log;
   friend class LogsCommand;
public:
   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   LogThread();

   //  Private because this singleton is not subclassed.
   //
   ~LogThread();

   //> When bundling logs into a stream, the threshold that prevents
   //  another log from being added to the stream.
   //
   static const size_t BundledLogSizeThreshold = 2048;

   //  Retrieves logs from BUFFER and bundles them into an ostringstream.
   //  Returns nullptr if BUFFER was empty.
   //
   static ostringstreamPtr GetLogsFromBuffer(LogBuffer* buffer);

   //  Invoked to immediately output LOG during a restart.  Writes LOG
   //  to the log file and, if appropriate, the console.  LOG is freed
   //  and set to nullptr before returning.
   //
   void Spool(ostringstreamPtr& log);

   //  Overridden to return a name for the thread.
   //
   const char* AbbrName() const override;

   //  Overridden to dequeue log requests.
   //
   void Enter() override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;

   //  Critical section lock for the log file.
   //
   SysMutex lock_;

   //  The configuration parameter for the number of MsgBuffers reserved
   //  for work other than spooling logs.
   //
   CfgIntParmPtr noSpoolingMessageCount_;

   //  The number of MsgBuffers reserved for work other than spooling logs.
   //
   static word NoSpoolingMessageCount_;
};
}
#endif
