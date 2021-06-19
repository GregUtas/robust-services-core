//==============================================================================
//
//  SnIncrement.cpp
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
#include "SnIncrement.h"
#include "CliCommand.h"
#include "PotsIncrement.h"
#include <sstream>
#include <string>
#include "CliIntParm.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "NbCliParms.h"
#include "PotsCliParms.h"
#include "PotsSessions.h"
#include "PotsTreatmentRegistry.h"
#include "PotsTreatments.h"
#include "Singleton.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
//  The SIZES command.
//
class SnSizesCommand : public PbSizesCommand
{
public:
   SnSizesCommand() = default;
private:
   void DisplaySizes(const CliThread& cli, bool all) const override;
   word ProcessCommand(CliThread& cli) const override;
};

void SnSizesCommand::DisplaySizes(const CliThread& cli, bool all) const
{
   if(all)
   {
      PbSizesCommand::DisplaySizes(cli, all);
      *cli.obuf << CRLF;
   }

   *cli.obuf << "  PotsBcSsm = " << sizeof(PotsBcSsm) << CRLF;
   *cli.obuf << "  PotsTreatment = " << sizeof(PotsTreatment) << CRLF;
}

word SnSizesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("SnSizesCommand.ProcessCommand");

   bool all = false;

   if(GetBoolParmRc(all, cli) == Error) return -1;
   if(!cli.EndOfInput()) return -1;
   *cli.obuf << spaces(2) << SizesHeader << CRLF;
   DisplaySizes(cli, all);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The TREATMENTS command.
//
fixed_string TreatmentQIdOptExpl = "PotsTreatmentRegistry::QId (default=all)";

class TreatmentsCommand : public CliCommand
{
public:
   TreatmentsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string TreatmentsStr = "treatments";
fixed_string TreatmentsExpl = "Displays treatments.";

TreatmentsCommand::TreatmentsCommand() :
   CliCommand(TreatmentsStr, TreatmentsExpl)
{
   BindParm(*new CliIntParm
      (TreatmentQIdOptExpl, 0, PotsTreatmentQueue::MaxQId, true));
   BindParm(*new DispBVParm);
}

word TreatmentsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("TreatmentsCommand.ProcessCommand");

   word qid;
   bool all, v = false;

   switch(GetIntParmRc(qid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton< PotsTreatmentRegistry >::Instance();

   if(all)
   {
      reg->Output(*cli.obuf, 2, v);
   }
   else
   {
      auto tq = reg->TreatmentQ(qid);
      if(tq == nullptr) return cli.Report(-2, NoTreatmentExpl);
      tq->Output(*cli.obuf, 2, v);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The Service Node increment.
//
fixed_string SnText = "sn";
fixed_string SnExpl = "Service Node Increment";

SnIncrement::SnIncrement() : CliIncrement(SnText, SnExpl)
{
   Debug::ft("SnIncrement.ctor");

   BindCommand(*new TreatmentsCommand);
   BindCommand(*new SnSizesCommand);
}

//------------------------------------------------------------------------------

SnIncrement::~SnIncrement()
{
   Debug::ftnt("SnIncrement.dtor");
}
}
