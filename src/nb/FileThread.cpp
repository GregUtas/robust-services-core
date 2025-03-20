//==============================================================================
//
//  FileThread.cpp
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
#include "FileThread.h"
#include "StreamRequest.h"
#include <ios>
#include <iosfwd>
#include <new>
#include <sstream>
#include "Debug.h"
#include "Duration.h"
#include "Element.h"
#include "FileSystem.h"
#include "FunctionGuard.h"
#include "Mutex.h"
#include "Restart.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  For serializing access to our message queue.
//
static Mutex FileThreadMsgQLock_("FileThreadMsgQLock");

//  For preventing interleaved output in the console transcript file.
//
static Mutex ConsoleFileLock_("ConsoleFileLock");

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

FileRequest::FileRequest(const string& name, bool trunc) :
   name_(nullptr),
   trunc_(trunc)
{
   Debug::ft("FileRequest.ctor");

   name_ = stringPtr(new string(name));
}

//------------------------------------------------------------------------------

FileRequest::~FileRequest()
{
   Debug::ftnt("FileRequest.dtor");
}

//------------------------------------------------------------------------------

FileRequest::FileRequest(const FileRequest& that) : StreamRequest(that),
   name_(nullptr),
   trunc_(that.trunc_)
{
   Debug::ft("FileRequest.ctor(copy)");

   name_ = stringPtr(new string(*that.name_));
}

//------------------------------------------------------------------------------

void FileRequest::Cleanup()
{
   Debug::ft("FileRequest.Cleanup");

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

FileThread::FileThread() : Thread(BackgroundFaction)
{
   Debug::ft("FileThread.ctor");

   SetInitialized();
}

//------------------------------------------------------------------------------

FileThread::~FileThread()
{
   Debug::ftnt("FileThread.dtor");
}

//------------------------------------------------------------------------------

c_string FileThread::AbbrName() const
{
   return "file";
}

//------------------------------------------------------------------------------

ostringstreamPtr FileThread::CreateStream()
{
   Debug::ft("FileThread.CreateStream");

   ostringstreamPtr stream(new std::ostringstream);
   *stream << std::boolalpha << std::nouppercase;
   return stream;
}

//------------------------------------------------------------------------------

void FileThread::Destroy()
{
   Debug::ft("FileThread.Destroy");

   Singleton<FileThread>::Destroy();
}

//------------------------------------------------------------------------------

void FileThread::Enter()
{
   Debug::ft("FileThread.Enter");

   while(true)
   {
      auto msg = DeqMsg(TIMEOUT_NEVER);
      auto req = static_cast<FileRequest*>(msg);

      if(req == nullptr) continue;
      stringPtr name(req->TakeName());
      ostringstreamPtr stream(req->TakeStream());
      CallbackRequestPtr written(req->TakeCallback());
      auto trunc = req->GetTrunc();

      delete msg;
      msg = nullptr;

      FunctionGuard guard(Guard_MakePreemptable);

      auto path = Element::OutputPath() + PATH_SEPARATOR + *name;
      auto file = FileSystem::CreateOstream(path.c_str(), trunc);

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

void FileThread::Record(const std::string& s, bool eol)
{
   Debug::ftnt("FileThread.Record");

   MutexGuard guard(&ConsoleFileLock_);

   auto name = Element::ConsoleFileName();
   Spool(name, s, eol);
}

//------------------------------------------------------------------------------

void FileThread::Spool(const string& name,
   ostringstreamPtr& stream, CallbackRequestPtr& written, bool trunc)
{
   Debug::ftnt("FileThread.Spool(written)");

   if(stream == nullptr) return;

   //  During a restart, our thread won't run, so output the stream directly.
   //
   if(Restart::GetStage() != Running)
   {
      auto path = Element::OutputPath() + PATH_SEPARATOR + name;
      auto file = FileSystem::CreateOstream(path.c_str(), trunc);

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
   Singleton<FileThread>::Instance()->EnqMsg(*request);
}

//------------------------------------------------------------------------------

void FileThread::Spool(const string& name, ostringstreamPtr& stream, bool trunc)
{
   Debug::ftnt("FileThread.Spool(stream)");

   CallbackRequestPtr callback;
   Spool(name, stream, callback, trunc);
}

//------------------------------------------------------------------------------

void FileThread::Spool(const string& name, const string& str, bool eol)
{
   Debug::ftnt("FileThread.Spool(string)");

   ostringstreamPtr stream(new (std::nothrow) std::ostringstream);
   if(stream == nullptr) return;

   *stream << str;
   if(eol) *stream << CRLF;

   CallbackRequestPtr callback;
   Spool(name, stream, callback);
}

//------------------------------------------------------------------------------

void FileThread::Truncate(const string& name)
{
   Debug::ft("FileThread.Truncate");

   auto path = Element::OutputPath() + PATH_SEPARATOR + name;
   auto file = FileSystem::CreateOstream(path.c_str(), true);
   file.reset();
}
}
