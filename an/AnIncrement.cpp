//==============================================================================
//
//  AnIncrement.cpp
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
#include "AnIncrement.h"
#include "CliCommand.h"
#include "CliText.h"
#include "CliTextParm.h"
#include <sstream>
#include "BcSessions.h"
#include "CliIntParm.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "NbCliParms.h"
#include "PotsCircuit.h"
#include "PotsTrafficThread.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  The TRAFFIC command.
//
class TrafficRateText : public CliText
{
public: TrafficRateText();
};

class TrafficAction : public CliTextParm
{
public: TrafficAction();
};

class TrafficCommand : public CliCommand
{
public:
   TrafficCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string TrafficStatesTextStr = "states";
fixed_string TrafficStatesTextExpl = "displays circuit and call states";

fixed_string CallRateExpl = "calls per minute";

fixed_string TrafficRateTextStr = "rate";
fixed_string TrafficRateTextExpl = "sets call rate";

TrafficRateText::TrafficRateText() :
   CliText(TrafficRateTextExpl, TrafficRateTextStr)
{
   BindParm(*new CliIntParm
      (CallRateExpl, 0, PotsTrafficThread::MaxCallsPerMin));
}

fixed_string TrafficQueryTextStr = "query";
fixed_string TrafficQueryTextExpl = "displays traffic statistics";

const id_t TrafficStatesIndex = 1;
const id_t TrafficRateIndex = 2;
const id_t TrafficQueryIndex = 3;

fixed_string TrafficActionExpl = "subcommand...";

TrafficAction::TrafficAction() : CliTextParm(TrafficActionExpl)
{
   BindText(*new CliText
      (TrafficStatesTextExpl, TrafficStatesTextStr), TrafficStatesIndex);
   BindText(*new TrafficRateText, TrafficRateIndex);
   BindText(*new CliText
      (TrafficQueryTextExpl, TrafficQueryTextStr), TrafficQueryIndex);
}

fixed_string TrafficStr = "traffic";
fixed_string TrafficExpl = "Generates POTS calls for load testing.";

TrafficCommand::TrafficCommand() : CliCommand(TrafficStr, TrafficExpl)
{
   BindParm(*new TrafficAction);
}

fn_name TrafficCommand_ProcessCommand = "TrafficCommand.ProcessCommand";

word TrafficCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(TrafficCommand_ProcessCommand);

   id_t index;
   word rate;

   if(!GetTextIndex(index, cli)) return -1;

   switch(index)
   {
   case TrafficStatesIndex:
      if(!cli.EndOfInput()) return -1;
      *cli.obuf << "Basic call states:" << CRLF;
      BcSsm::DisplayStateCounts(*cli.obuf, spaces(2));
      *cli.obuf << "POTS circuit states:" << CRLF;
      PotsCircuit::DisplayStateCounts(*cli.obuf, spaces(2));
      *cli.obuf << "Traffic call states:" << CRLF;
      PotsTrafficThread::DisplayStateCounts(*cli.obuf, spaces(2));
      break;

   case TrafficRateIndex:
      if(!GetIntParm(rate, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      Singleton< PotsTrafficThread >::Instance()->SetRate(rate);
      break;

   case TrafficQueryIndex:
      if(!cli.EndOfInput()) return -1;
      Singleton< PotsTrafficThread >::Instance()->Query(*cli.obuf);
      break;

   default:
      Debug::SwLog(TrafficCommand_ProcessCommand, UnexpectedIndex, index);
      return cli.Report(index, SystemErrorExpl);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The AccessNode increment.
//
fixed_string AnText = "an";
fixed_string AnExpl = "Access Node Increment";

AnIncrement::AnIncrement() : CliIncrement(AnText, AnExpl)
{
   Debug::ft("AnIncrement.ctor");

   BindCommand(*new TrafficCommand);
}

//------------------------------------------------------------------------------

AnIncrement::~AnIncrement()
{
   Debug::ftnt("AnIncrement.dtor");
}
}
