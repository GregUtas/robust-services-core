//==============================================================================
//
//  FileThread.cpp
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
#include "FileThread.h"
#include "StreamRequest.h"
#include <ios>
#include <iosfwd>
#include <sstream>
#include "Clock.h"
#include "Debug.h"
#include "Element.h"
#include "FunctionGuard.h"
#include "MutexGuard.h"
#include "Restart.h"
#include "Singleton.h"
#include "SysFile.h"
#include "SysMutex.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  For serializing access to our message queue.
//
SysMutex FileThreadMsgQLock_("FileThreadMsgQLock");

//  For preventing interleaved output in the console transcript file.
//
SysMutex ConsoleFileLock_("ConsoleFileLock");

//------------------------------------------------------------------------------
//
//  For queueing output to a file.
//
class FileRequest : public StreamRequest
{
public:
   //  Creates a request to be written to a file called NAME.  If TRUNC
   //  is set, the file is overwritten instead of appended to.
   //
   FileRequest(const string& name, bool trunc);

   //  Copy constructor.
   //
   FileRequest(const FileRequest& that);

   //  Deletes the filename if still owned.
   //
   ~FileRequest();

   //  Takes ownership of the filename from the request.
   //
   string* TakeName() { return name_.release(); }

   //  Returns true if the named file should be overwritten.
   //
   bool GetTrunc() const { return trunc_; }

   //  Overridden to display member variables.
   //
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Overridden to delete name_ during error recovery.
   //
   void Cleanup() override;

   //  The name of the file where the output is to be placed.
   //
   stringPtr name_;

   //  Set if an existing file is to be overwritten.
   //
   const bool trunc_;
};

//------------------------------------------------------------------------------

fn_name FileRequest_ctor1 = "FileRequest.ctor";

FileRequest::FileRequest(const string& name, bool trunc) :
   name_(nullptr),
   trunc_(trunc)
{
   Debug::ft(FileRequest_ctor1);

   name_ = stringPtr(new string(name));
}

//------------------------------------------------------------------------------

fn_name FileRequest_ctor2 = "FileRequest.ctor(copy)";

FileRequest::FileRequest(const FileRequest& that) : StreamRequest(that),
   name_(nullptr),
   trunc_(that.trunc_)
{
   Debug::ft(FileRequest_ctor2);

   name_ = stringPtr(new string(*that.name_));
}

//------------------------------------------------------------------------------

fn_name FileRequest_dtor = "FileRequest.dtor";

FileRequest::~FileRequest()
{
   Debug::ft(FileRequest_dtor);
}

//------------------------------------------------------------------------------

fn_name FileRequest_Cleanup = "FileRequest.Cleanup";

void FileRequest::Cleanup()
{
   Debug::ft(FileRequest_Cleanup);

   name_.reset();
   StreamRequest::Cleanup();
}

//------------------------------------------------------------------------------

void FileRequest::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   StreamRequest::Display(stream, prefix, options);

   stream << prefix << "name  : " << *name_ << CRLF;
   stream << prefix << "trunc : " << trunc_ << CRLF;
}

//------------------------------------------------------------------------------

void FileRequest::Patch(sel_t selector, void* arguments)
{
   StreamRequest::Patch(selector, arguments);
}

//==============================================================================

fn_name FileThread_ctor = "FileThread.ctor";

FileThread::FileThread() : Thread(BackgroundFaction)
{
   Debug::ft(FileThread_ctor);

   SetInitialized();
}

//------------------------------------------------------------------------------

fn_name FileThread_dtor = "FileThread.dtor";

FileThread::~FileThread()
{
   Debug::ft(FileThread_dtor);
}

//------------------------------------------------------------------------------

c_string FileThread::AbbrName() const
{
   return "file";
}

//------------------------------------------------------------------------------

fn_name FileThread_CreateStream = "FileThread.CreateStream";

ostringstreamPtr FileThread::CreateStream()
{
   Debug::ft(FileThread_CreateStream);

   ostringstreamPtr stream(new std::ostringstream);
   *stream << std::boolalpha << std::nouppercase;
   return stream;
}

//------------------------------------------------------------------------------

fn_name FileThread_Destroy = "FileThread.Destroy";

void FileThread::Destroy()
{
   Debug::ft(FileThread_Destroy);

   Singleton< FileThread >::Destroy();
}

//------------------------------------------------------------------------------

fn_name FileThread_Enter = "FileThread.Enter";

void FileThread::Enter()
{
   Debug::ft(FileThread_Enter);

   while(true)
   {
      auto msg = DeqMsg(TIMEOUT_NEVER);
      auto req = static_cast< FileRequest* >(msg);

      if(req == nullptr) continue;
      stringPtr name(req->TakeName());
      ostringstreamPtr stream(req->TakeStream());
      CallbackRequestPtr written(req->TakeCallback());
      auto trunc = req->GetTrunc();

      delete msg;
      msg = nullptr;

      FunctionGuard guard(FunctionGuard::MakePreemptable);

      auto path = Element::OutputPath() + PATH_SEPARATOR + *name;
      auto file = SysFile::CreateOstream(path.c_str(), trunc);

      if(file != nullptr)
      {
         *file << stream->str();
         file.reset();
      }

      //  Now that the file has been written, run unpreemptably
      //  again before invoking any callback.
      //
      guard.Release();
      if(written != nullptr) written->Callback();
      name.reset();
      stream.reset();
      written.reset();
   }
}

//------------------------------------------------------------------------------

void FileThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name FileThread_Record = "FileThread.Record";

void FileThread::Record(const std::string& s, bool eol)
{
   Debug::ft(FileThread_Record);

   MutexGuard guard(&ConsoleFileLock_);

   auto name = Element::ConsoleFileName() + ".txt";
   Spool(name, s, eol);
}

//------------------------------------------------------------------------------

fn_name FileThread_Spool1 = "FileThread.Spool(written)";

void FileThread::Spool(const string& name,
   ostringstreamPtr& stream, CallbackRequestPtr& written, bool trunc)
{
   Debug::ft(FileThread_Spool1);

   if(stream == nullptr) return;

   //  During a restart, our thread won't run, so output the stream directly.
   //
   if(Restart::GetStatus() != Running)
   {
      auto path = Element::OutputPath() + PATH_SEPARATOR + name;
      auto file = SysFile::CreateOstream(path.c_str(), trunc);

      if(file != nullptr)
      {
         *file << stream->str();
         file.reset();
      }

      if(written != nullptr) written->Callback();
      stream.reset();
      written.reset();
      return;
   }

   //  Forward the stream to our thread.
   //
   auto request = new FileRequest(name, trunc);
   request->GiveStream(stream);
   request->GiveCallback(written);

   //  This function runs on the client thread, so it contends for our
   //  message queue with our Enter function.  Although it's unlikely,
   //  the client could be preemptable or of higher priority.
   //
   MutexGuard guard(&FileThreadMsgQLock_);
   Singleton< FileThread >::Instance()->EnqMsg(*request);
}

//------------------------------------------------------------------------------

fn_name FileThread_Spool2 = "FileThread.Spool(stream)";

void FileThread::Spool(const string& name, ostringstreamPtr& stream, bool trunc)
{
   Debug::ft(FileThread_Spool2);

   CallbackRequestPtr callback;
   Spool(name, stream, callback, trunc);
}

//------------------------------------------------------------------------------

fn_name FileThread_Spool3 = "FileThread.Spool(string)";

void FileThread::Spool(const string& name, const string& s, bool eol)
{
   Debug::ft(FileThread_Spool3);

   ostringstreamPtr stream(new std::ostringstream);
   *stream << s;
   if(eol) *stream << CRLF;

   CallbackRequestPtr callback;
   Spool(name, stream, callback);
}

//------------------------------------------------------------------------------

fn_name FileThread_Truncate = "FileThread.Truncate";

void FileThread::Truncate(const string& name)
{
   Debug::ft(FileThread_Truncate);

   auto path = Element::OutputPath() + PATH_SEPARATOR + name;
   auto file = SysFile::CreateOstream(path.c_str(), true);
   file.reset();
}
}
