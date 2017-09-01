//==============================================================================
//
//  SnIncrement.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "SnIncrement.h"
#include <sstream>
#include <string>
#include "CliCommand.h"
#include "CliIntParm.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "NbCliParms.h"
#include "PotsCliParms.h"
#include "PotsIncrement.h"
#include "PotsSessions.h"
#include "PotsTreatmentRegistry.h"
#include "PotsTreatments.h"
#include "Singleton.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
class TreatmentQIdOptParm : public CliIntParm
{
public: TreatmentQIdOptParm();
};

fixed_string TreatmentQIdOptExpl = "PotsTreatmentRegistry::QId (default=all)";

TreatmentQIdOptParm::TreatmentQIdOptParm() :
   CliIntParm(TreatmentQIdOptExpl, 0, PotsTreatmentQueue::MaxQId, true) { }

//------------------------------------------------------------------------------
//
//  The SIZES command.
//
class SnSizesCommand : public PbSizesCommand
{
public:
   SnSizesCommand() { }
private:
   virtual word ProcessCommand(CliThread& cli) const override;
   virtual void DisplaySizes(CliThread& cli, bool all) const override;
};

void SnSizesCommand::DisplaySizes(CliThread& cli, bool all) const
{
   if(all)
   {
      PbSizesCommand::DisplaySizes(cli, all);
      *cli.obuf << CRLF;
   }

   *cli.obuf << "  PotsBcSsm = " << sizeof(PotsBcSsm) << CRLF;
   *cli.obuf << "  PotsTreatment = " << sizeof(PotsTreatment) << CRLF;
}

fn_name SnSizesCommand_ProcessCommand = "SnSizesCommand.ProcessCommand";

word SnSizesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(SnSizesCommand_ProcessCommand);

   bool all = false;

   if(GetBoolParmRc(all, cli) == Error) return -1;
   cli.EndOfInput(false);
   *cli.obuf << spaces(2) << SizesHeader << CRLF;
   DisplaySizes(cli, all);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The TREATMENTS command.
//
class TreatmentsCommand : public CliCommand
{
public:
   TreatmentsCommand();
private:
   virtual word ProcessCommand(CliThread& cli) const override;
};

fixed_string TreatmentsStr = "treatments";
fixed_string TreatmentsExpl = "Displays treatments.";

TreatmentsCommand::TreatmentsCommand() :
   CliCommand(TreatmentsStr, TreatmentsExpl)
{
   BindParm(*new TreatmentQIdOptParm);
   BindParm(*new DispBVParm);
}

fn_name TreatmentsCommand_ProcessCommand = "TreatmentsCommand.ProcessCommand";

word TreatmentsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(TreatmentsCommand_ProcessCommand);

   word qid;
   bool all, v = false;

   switch(GetIntParmRc(qid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   cli.EndOfInput(false);

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

fn_name SnIncrement_ctor = "SnIncrement.ctor";

SnIncrement::SnIncrement() : CliIncrement(SnText, SnExpl)
{
   Debug::ft(SnIncrement_ctor);

   BindCommand(*new TreatmentsCommand);
   BindCommand(*new SnSizesCommand);
}

//------------------------------------------------------------------------------

fn_name SnIncrement_dtor = "SnIncrement.dtor";

SnIncrement::~SnIncrement()
{
   Debug::ft(SnIncrement_dtor);
}
}
