//==============================================================================
//
//  TimerProtocol.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef TIMERPROTOCOL_H_INCLUDED
#define TIMERPROTOCOL_H_INCLUDED

#include "TlvProtocol.h"
#include <iosfwd>
#include <string>
#include "NbTypes.h"
#include "SbTypes.h"
#include "Signal.h"
#include "TlvParameter.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  The timer protocol defines the timeout signal and parameter.  All protocols
//  should inherit it, which they can do by passing TimerProtocolId as the BASE
//  argument to Protocol's constructor.
//
class TimerProtocol : public TlvProtocol
{
   friend class Singleton< TimerProtocol >;
private:
   //  Private because this singleton is not subclassed.
   //
   TimerProtocol();

   //  Private because this singleton is not subclassed.
   //
   ~TimerProtocol();
};

//------------------------------------------------------------------------------
//
//  The signal for a timeout message.
//
class TimeoutSignal : public Signal
{
   friend class Singleton< TimeoutSignal >;
private:
   //  Private because this singleton is not subclassed.
   //
   TimeoutSignal();

   //  Private because this singleton is not subclassed.
   //
   ~TimeoutSignal();
};

//------------------------------------------------------------------------------
//
//  The parameter found in a timeout message.
//
struct TimeoutInfo
{
   const Base* owner;    // as originally passed to ProtocolSM::StartTimer
   TimerId tid;          // as originally passed to ProtocolSM::StartTimer

   TimeoutInfo();
   void Display(std::ostream& stream, const std::string& prefix) const;
};

//------------------------------------------------------------------------------
//
//  The parameter for a timeout message.
//
class TimeoutParameter : public TlvParameter
{
   friend class Singleton< TimeoutParameter >;
public:
   //  Overridden to display the parameter symbolically.
   //
   virtual void DisplayMsg(std::ostream& stream, const std::string& prefix,
      const byte_t* bytes, size_t count) const override;
private:
   //  Private because this singleton is not subclassed.
   //
   TimeoutParameter();

   //  Private because this singleton is not subclassed.
   //
   ~TimeoutParameter();
};
}
#endif
