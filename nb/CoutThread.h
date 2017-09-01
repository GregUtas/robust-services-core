//==============================================================================
//
//  CoutThread.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
   //  of STREAM, which is set to nullptr before returning.
   //
   static void Spool(ostringstreamPtr& stream);

   //  Queues STR for output to the console.  Adds an "CRLF" if EOL is set.
   //
   static void Spool(const char* s, bool eol = false);

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   CoutThread();

   //  Private because this singleton is not subclassed.
   //
   ~CoutThread();

   //  Overridden to return a name for the thread.
   //
   virtual const char* AbbrName() const override;

   //  Overridden to dequeue console output requests.
   //
   virtual void Enter() override;

   //  Overridden to delete the singleton.
   //
   virtual void Destroy() override;
};
}
#endif
