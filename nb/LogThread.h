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
#include "CallbackRequest.h"
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

   //> When bundling logs into a stream, the number of characters that
   //  prevents another log from being added to the stream.
   //
   static const size_t BundledLogSizeThreshold;

   //  Retrieves logs from BUFFER and bundles them into an ostringstream.
   //  Returns nullptr if BUFFER was empty, else updates CALLBACK so that
   //  BUFFER can free the space occupied by the logs after they have been
   //  written.
   //
   static ostringstreamPtr GetLogsFromBuffer
      (LogBuffer* buffer, CallbackRequestPtr& callback);

   //  Invoked to immediately output a STREAM of logs during a restart.
   //  Writes STREAM to the log file and, if appropriate, the console.
   //  STREAM is freed and set to nullptr before returning.
   //
   static void Spool(ostringstreamPtr& stream);

   //  Copies the STREAM of logs to the console when appropriate.
   //
   static void CopyToConsole(const ostringstreamPtr& stream);

   //  Overridden to return a name for the thread.
   //
   c_string AbbrName() const override;

   //  Overridden to dequeue log requests.
   //
   void Enter() override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;

   //  The configuration parameter for the number of MsgBuffers reserved
   //  for work other than spooling logs.
   //
   CfgIntParmPtr noSpoolingMessageCount_;

   //  The number of MsgBuffers reserved for work other than spooling logs.
   //
   static word NoSpoolingMessageCount_;

   //  To prevent interleaved output in the log file.
   //
   static SysMutex LogFileLock_;
};
}
#endif
