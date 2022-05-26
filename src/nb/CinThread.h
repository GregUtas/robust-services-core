//==============================================================================
//
//  CinThread.h
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
#ifndef CINTHREAD_H_INCLUDED
#define CINTHREAD_H_INCLUDED

#include "Thread.h"
#include <ios>
#include <string>
#include "NbTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Thread for console input.  This will eventually evolve to support
//  a remote console that sends input via an IP port.
//
class CinThread : public Thread
{
   friend class Singleton<CinThread>;
public:
   //  Reads input from the console and places it in BUFF.  Returns the number
   //  of characters read (N >= 1); note that an endline is appended to BUFF.
   //  If N <= 0, see StreamRc.  The client is only scheduled out if input is
   //  not yet available, so it must not call EnterBlockingOperation before it
   //  invokes this function.
   //
   static std::streamsize GetLine(std::string& buff);

   //  If THR is the current client, clears it.  A thread's destructor should
   //  invoke this if it uses CinThread::GetLine.
   //
   void ClearClient(const Thread* client);

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
   CinThread();

   //  Private because this is a singleton.
   //
   ~CinThread();

   //  Registers CLIENT as waiting for input.  Returns false if another
   //  client is currently registered.
   //
   bool SetClient(Thread* client);

   //  Overridden to return a name for the thread.
   //
   c_string AbbrName() const override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;

   //  Overridden to read input from the console and either buffer it
   //  or pass it to a waiting thread.
   //
   void Enter() override;

   //  Buffer for input.
   //
   std::string buff_;

   //  The thread that is waiting for input.
   //
   Thread* client_;
};
}
#endif
