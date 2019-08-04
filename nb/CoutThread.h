//==============================================================================
//
//  CoutThread.h
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
#ifndef COUTTHREAD_H_INCLUDED
#define COUTTHREAD_H_INCLUDED

#include "Thread.h"
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Thread for console output.  All threads use this to prevent interleaved
//  gibberish on the console.
//
class CoutThread : public Thread
{
   friend class Singleton< CoutThread >;
public:
   //  Queues STREAM for output to the console.  CoutThread takes ownership
   //  of STREAM, which is set to nullptr before returning.  STREAM is also
   //  copied and sent to the console transcript file.
   //
   static void Spool(ostringstreamPtr& stream);

   //  Queues STR for output to the console.  Adds a "CRLF" if EOL is set.
   //
   static void Spool(c_string s, bool eol = false);

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   CoutThread();

   //  Private because this singleton is not subclassed.
   //
   ~CoutThread();

   //  Overridden to return a name for the thread.
   //
   c_string AbbrName() const override;

   //  Overridden to dequeue console output requests.
   //
   void Enter() override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;
};
}
#endif
