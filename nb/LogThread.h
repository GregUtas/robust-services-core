//==============================================================================
//
//  LogThread.h
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
#ifndef LOGTHREAD_H_INCLUDED
#define LOGTHREAD_H_INCLUDED

#include "Thread.h"
#include <cstddef>
#include "CfgIntParm.h"
#include "NbTypes.h"
#include "SysTypes.h"

namespace NodeBase
{
   class Log;
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
public:
   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   LogThread();

   //  Private because this is a singleton.
   //
   ~LogThread();

   //  Invoked to immediately output STREAM, which contains a log of type
   //  LOG, during a restart.  STREAM is freed and set to nullptr before
   //  returning.
   //
   static void Spool(ostringstreamPtr& stream, const Log* log);

   //  Returns the number of message buffers reserved for work other than
   //  spooling logs.
   //
   size_t NoSpoolingMessageCount() const
      { return noSpoolingMessageCount_->GetValue(); }

   //  Overridden to return a name for the thread.
   //
   c_string AbbrName() const override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;

   //  Overridden to dequeue log requests.
   //
   void Enter() override;

   //  The configuration parameter for the number of MsgBuffers reserved
   //  for work other than spooling logs.
   //
   CfgIntParmPtr noSpoolingMessageCount_;
};
}
#endif
