//==============================================================================
//
//  TimerProtocol.h
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
#ifndef TIMERPROTOCOL_H_INCLUDED
#define TIMERPROTOCOL_H_INCLUDED

#include "TlvProtocol.h"
#include <iosfwd>
#include <string>
#include "NbTypes.h"
#include "SbTypes.h"
#include "Signal.h"
#include "TlvParameter.h"

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
