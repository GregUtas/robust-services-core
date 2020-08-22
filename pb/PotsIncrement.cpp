//==============================================================================
//
//  PotsIncrement.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "PotsIncrement.h"
#include "CliCommand.h"
#include "CliIntParm.h"
#include <sstream>
#include <string>
#include "BcAddress.h"
#include "BcCause.h"
#include "BcProgress.h"
#include "CliTextParm.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "LocalAddress.h"
#include "MbPools.h"
#include "MediaEndpt.h"
#include "MediaParameter.h"
#include "NbCliParms.h"
#include "PotsCircuit.h"
#include "PotsCliParms.h"
#include "PotsFeature.h"
#include "PotsFeatureProfile.h"
#include "PotsFeatureRegistry.h"
#include "PotsProfile.h"
#include "PotsProfileRegistry.h"
#include "PotsProtocol.h"
#include "SbCliParms.h"
#include "ServiceCodeRegistry.h"
#include "Singleton.h"
#include "Switch.h"
#include "SysTypes.h"
#include "ThisThread.h"
#include "ToneRegistry.h"
#include "Tones.h"

using namespace SessionBase;
using namespace MediaBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  Parameters that support basic types.
//
class PortMandParm : public CliIntParm
{
public: PortMandParm();
};

class PortOptParm : public CliIntParm
{
public: PortOptParm();
};

class PotsFeatureOptParm : public CliIntParm
{
public: PotsFeatureOptParm();
};

class ToneOptParm : public CliIntParm
{
public: ToneOptParm();
};

fixed_string PortExpl = "Switch::PortId";

PortMandParm::PortMandParm() : CliIntParm(PortExpl, 0, Switch::MaxPortId) { }

PortOptParm::PortOptParm() :
   CliIntParm(PortExpl, 0, Switch::MaxPortId, true) { }

fixed_string PotsFeatureOptExpl = "PotsFeature::Id";

PotsFeatureOptParm::PotsFeatureOptParm() :
   CliIntParm(PotsFeatureOptExpl, 0, PotsFeature::MaxId, true) { }

fixed_string ToneOptExpl = "Tone::Id (default=all)";

ToneOptParm::ToneOptParm() :
   CliIntParm(ToneOptExpl, 0, Tone::MaxId, true) { }

//------------------------------------------------------------------------------
//
//  The ACTIVATE command.
//
class ActivateCommand : public CliCommand
{
public:
   ActivateCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ActivateStr = "activate";
fixed_string ActivateExpl = "Activates a feature assigned to a DN.";

ActivateCommand::ActivateCommand() : CliCommand(ActivateStr, ActivateExpl)
{
   BindParm(*new DnMandParm);
   BindParm(*Singleton< PotsFeatureRegistry >::Instance()->featuresActivate_);
}

fn_name ActivateCommand_ProcessCommand = "ActivateCommand.ProcessCommand";

word ActivateCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(ActivateCommand_ProcessCommand);

   word id1;
   id_t id2;

   if(!GetIntParm(id1, cli)) return -1;

   auto pro = Singleton< PotsProfileRegistry >::Instance()->Profile(id1);
   if(pro == nullptr) return cli.Report(-3, NotRegisteredExpl);

   if(!GetTextIndex(id2, cli)) return -1;

   auto ftr = pro->FindFeature(id2);
   if(ftr == nullptr) return cli.Report(-3, NotSubscribedExpl);
   if(!ftr->Activate(*pro, cli)) return -4;
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The CODES command.
//
class CodesCommand : public CliCommand
{
public:
   CodesCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string CodesStr = "codes";
fixed_string CodesExpl = "Displays service codes.";

CodesCommand::CodesCommand() : CliCommand(CodesStr, CodesExpl) { }

fn_name CodesCommand_ProcessCommand = "CodesCommand.ProcessCommand";

word CodesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(CodesCommand_ProcessCommand);

   if(!cli.EndOfInput()) return -1;

   Singleton< ServiceCodeRegistry >::Instance()->Output(*cli.obuf, 2, true);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The DEACTIVATE command.
//
class DeactivateCommand : public CliCommand
{
public:
   DeactivateCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string DeactivateStr = "deactivate";
fixed_string DeactivateExpl = "Deactivates a feature assigned to a DN.";

DeactivateCommand::DeactivateCommand() :
   CliCommand(DeactivateStr, DeactivateExpl)
{
   auto reg = Singleton< PotsFeatureRegistry >::Instance();
   BindParm(*new DnMandParm);
   BindParm(*reg->featuresDeactivate_);
}

fn_name DeactivateCommand_ProcessCommand = "DeactivateCommand.ProcessCommand";

word DeactivateCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(DeactivateCommand_ProcessCommand);

   word id1;
   id_t id2;

   if(!GetIntParm(id1, cli)) return -1;

   auto pro = Singleton< PotsProfileRegistry >::Instance()->Profile(id1);
   if(pro == nullptr) return cli.Report(-3, NotRegisteredExpl);

   if(!GetTextIndex(id2, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto ftr = pro->FindFeature(id2);
   if(ftr == nullptr) return cli.Report(-3, NotSubscribedExpl);
   if(!ftr->Deactivate(*pro)) return -4;
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The DEREGISTER command.
//
class DeregisterCommand : public CliCommand
{
public:
   DeregisterCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string DeregisterStr = "deregister";
fixed_string DeregisterExpl = "Deletes a DN.";

DeregisterCommand::DeregisterCommand() :
   CliCommand(DeregisterStr, DeregisterExpl)
{
   BindParm(*new DnMandParm);
}

fn_name DeregisterCommand_ProcessCommand = "DeregisterCommand.ProcessCommand";

word DeregisterCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(DeregisterCommand_ProcessCommand);

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pro = Singleton< PotsProfileRegistry >::Instance()->Profile(id1);
   if(pro == nullptr) return cli.Report(-3, NotRegisteredExpl);
   if(!pro->Deregister()) return -4;
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The DNS command.
//
class DnsCommand : public CliCommand
{
public:
   DnsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string DnsStr = "dns";
fixed_string DnsExpl = "Displays the profile(s) in a range of DNs.";

DnsCommand::DnsCommand() : CliCommand(DnsStr, DnsExpl)
{
   BindParm(*new DnMandParm);
   BindParm(*new DnOptParm);
}

fn_name DnsCommand_ProcessCommand = "DnsCommand.ProcessCommand";

word DnsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(DnsCommand_ProcessCommand);

   word first, last;
   bool one = false;

   if(!GetIntParm(first, cli)) return -1;

   switch(GetIntParmRc(last, cli))
   {
   case None:
      last = first;
      //  [[ fallthrough]]
   case Ok:
      break;
   default:
      return -1;
   }

   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton< PotsProfileRegistry >::Instance();

   for(auto p = reg->FirstProfile(first); p != nullptr; NO_OP)
   {
      auto dn = p->GetDN();

      if(word(dn) <= last)
      {
         one = true;

         if(first == last)
         {
            p->Output(*cli.obuf, 2, true);
            return 0;
         }

         *cli.obuf << spaces(2) << strIndex(dn) << CRLF;
         p->Output(*cli.obuf, 4, false);
         p = reg->NextProfile(*p);
      }
      else
      {
         p = nullptr;
      }
   }

   if(!one) return cli.Report(-2, NoDnsExpl);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The FEATURES command.
//
class FeaturesCommand : public CliCommand
{
public:
   FeaturesCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string FeaturesStr = "features";
fixed_string FeaturesExpl = "Displays features that can be assigned to a DN.";

FeaturesCommand::FeaturesCommand() : CliCommand(FeaturesStr, FeaturesExpl)
{
   BindParm(*new PotsFeatureOptParm);
   BindParm(*new DispBVParm);
}

fn_name FeaturesCommand_ProcessCommand = "FeaturesCommand.ProcessCommand";

word FeaturesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(FeaturesCommand_ProcessCommand);

   word id;
   bool all, v = false;

   switch(GetIntParmRc(id, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton< PotsFeatureRegistry >::Instance();

   if(all)
   {
      reg->Output(*cli.obuf, 2, v);
   }
   else
   {
      auto ftr = reg->Feature(id);
      if(ftr == nullptr) return cli.Report(-2, NoFeatureExpl);
      ftr->Output(*cli.obuf, 4, v);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The MEPS command.
//
class MepsCommand : public CliCommand
{
public:
   MepsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string MepsStr = "meps";
fixed_string MepsExpl = "Counts or displays media endpoints.";

MepsCommand::MepsCommand() : CliCommand(MepsStr, MepsExpl)
{
   BindParm(*new FactoryIdOptParm);
   BindParm(*new DispCBVParm);
}

fn_name MepsCommand_ProcessCommand = "MepsCommand.ProcessCommand";

word MepsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(MepsCommand_ProcessCommand);

   word fid;
   bool all, c, v;

   switch(GetIntParmRc(fid, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetCBV(*this, cli, c, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pool = Singleton< MediaEndptPool >::Instance();

   if(c)
   {
      auto num = pool->InUseCount();
      *cli.obuf << spaces(2) << num << CRLF;
      return num;
   }

   PooledObjectId id;
   word count = 0;
   auto time = 200;

   for(auto obj = pool->FirstUsed(id); obj != nullptr; obj = pool->NextUsed(id))
   {
      auto mep = static_cast< MediaEndpt* >(obj);

      ++count;

      if(all || (mep->Psm()->GetFactory() == fid))
      {
         if(all)
         {
            *cli.obuf << spaces(2) << strObj(mep) << CRLF;
            --time;
         }
         else
         {
            mep->Output(*cli.obuf, 2, v);
            time -= 25;
         }

         if(time <= 0)
         {
            ThisThread::Pause();
            time = 200;
         }
      }
   }

   if(count == 0) return cli.Report(-2, NoMepsExpl);
   return count;
}

//------------------------------------------------------------------------------
//
//  The REGISTER command.
//
class RegisterCommand : public CliCommand
{
public:
   RegisterCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string RegisterStr = "register";
fixed_string RegisterExpl = "Adds a new DN.";

RegisterCommand::RegisterCommand() : CliCommand(RegisterStr, RegisterExpl)
{
   BindParm(*new DnMandParm);
}

fn_name RegisterCommand_ProcessCommand = "RegisterCommand.ProcessCommand";

word RegisterCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(RegisterCommand_ProcessCommand);

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pro = Singleton< PotsProfileRegistry >::Instance()->Profile(id1);
   if(pro != nullptr) return cli.Report(-3, AlreadyRegistered);

   FunctionGuard guard(Guard_MemUnprotect);
   pro = new PotsProfile(id1);
   if(pro == nullptr) return cli.Report(-7, AllocationError);
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The RESET command.
//
class ResetCommand : public CliCommand
{
public:
   ResetCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string ResetStr = "reset";
fixed_string ResetExpl = "Resets a DN to its initial state.";

ResetCommand::ResetCommand() : CliCommand(ResetStr, ResetExpl)
{
   BindParm(*new DnMandParm);
}

fn_name ResetCommand_ProcessCommand = "ResetCommand.ProcessCommand";

word ResetCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(ResetCommand_ProcessCommand);

   word id1;

   if(!GetIntParm(id1, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   auto pro = Singleton< PotsProfileRegistry >::Instance()->Profile(id1);
   if(pro == nullptr) return cli.Report(-3, NotRegisteredExpl);

   auto cct = pro->GetCircuit();
   if(cct == nullptr) return cli.Report (-3, NoCircuitExpl);

   auto msg = new Pots_NU_Message(nullptr, 20);
   PotsHeaderInfo phi;
   CauseInfo cci;

   phi.signal = PotsSignal::Release;
   phi.port = cct->TsPort();
   cci.cause = Cause::ResetCircuit;
   msg->AddHeader(phi);
   msg->AddCause(cci);

   cct->Profile()->ClearObjAddr(LocalAddress());

   if(!msg->Send(Message::External)) return cli.Report(-6, SendFailure);
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The SIZES command.
//
void PbSizesCommand::DisplaySizes(CliThread& cli, bool all) const
{
   if(all)
   {
      StSizesCommand::DisplaySizes(cli, all);
      *cli.obuf << CRLF;
   }

   *cli.obuf << "  CauseInfo = " << sizeof(CauseInfo) << CRLF;
   *cli.obuf << "  Circuit = " << sizeof(Circuit) << CRLF;
   *cli.obuf << "  DigitString = " << sizeof(DigitString) << CRLF;
   *cli.obuf << "  MediaInfo = " << sizeof(MediaInfo) << CRLF;
   *cli.obuf << "  ProgressInfo = " << sizeof(ProgressInfo) << CRLF;
   *cli.obuf << "  Switch = " << sizeof(Switch) << CRLF;

   *cli.obuf << "  Pots_UN_Message = " << sizeof(Pots_UN_Message) << CRLF;
   *cli.obuf << "  Pots_NU_Message = " << sizeof(Pots_NU_Message) << CRLF;
   *cli.obuf << "  PotsCallPsm = " << sizeof(PotsCallPsm) << CRLF;
   *cli.obuf << "  PotsCircuit = " << sizeof(PotsCircuit) << CRLF;
   *cli.obuf << "  PotsFeature = " << sizeof(PotsFeature) << CRLF;
   *cli.obuf << "  PotsFeatureProfile = " << sizeof(PotsFeatureProfile) << CRLF;
   *cli.obuf << "  PotsHeaderInfo = " << sizeof(PotsHeaderInfo) << CRLF;
   *cli.obuf << "  PotsProfile = " << sizeof(PotsProfile) << CRLF;
   *cli.obuf << "  PotsProfileRegistry = "
      << sizeof(PotsProfileRegistry) << CRLF;
   *cli.obuf << "  PotsRingInfo = " << sizeof(PotsRingInfo) << CRLF;
   *cli.obuf << "  PotsScanInfo = " << sizeof(PotsScanInfo) << CRLF;
}

fn_name PbSizesCommand_ProcessCommand = "PbSizesCommand.ProcessCommand";

word PbSizesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(PbSizesCommand_ProcessCommand);

   bool all = false;

   if(GetBoolParmRc(all, cli) == Error) return -1;
   if(!cli.EndOfInput()) return -1;
   *cli.obuf << spaces(2) << SizesHeader << CRLF;
   DisplaySizes(cli, all);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The SUBSCRIBE command.
//
class SubscribeCommand : public CliCommand
{
public:
   SubscribeCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string SubscribeStr = "subscribe";
fixed_string SubscribeExpl = "Assigns a feature to a DN.";

SubscribeCommand::SubscribeCommand() : CliCommand(SubscribeStr, SubscribeExpl)
{
   auto reg = Singleton< PotsFeatureRegistry >::Instance();
   BindParm(*new DnMandParm);
   BindParm(*reg->featuresSubscribe_);
}

fn_name SubscribeCommand_ProcessCommand = "SubscribeCommand.ProcessCommand";

word SubscribeCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(SubscribeCommand_ProcessCommand);

   word id1;
   id_t id2;

   if(!GetIntParm(id1, cli)) return -1;

   auto pro = Singleton< PotsProfileRegistry >::Instance()->Profile(id1);
   if(pro == nullptr) return cli.Report(-3, NotRegisteredExpl);

   if(!GetTextIndex(id2, cli)) return -1;

   if(pro->FindFeature(id2) != nullptr)
      return cli.Report(-3, AlreadySubscribed);
   if(!pro->Subscribe(id2, cli)) return -4;
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The TONES command.
//
class TonesCommand : public CliCommand
{
public:
   TonesCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string TonesStr = "tones";
fixed_string TonesExpl = "Displays tones.";

TonesCommand::TonesCommand() : CliCommand(TonesStr, TonesExpl)
{
   BindParm(*new ToneOptParm);
   BindParm(*new DispBVParm);
}

fn_name TonesCommand_ProcessCommand = "TonesCommand.ProcessCommand";

word TonesCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(TonesCommand_ProcessCommand);

   word id;
   bool all, v = false;

   switch(GetIntParmRc(id, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton< ToneRegistry >::Instance();

   if(all)
   {
      reg->Output(*cli.obuf, 2, v);
   }
   else
   {
      auto tone = reg->GetTone(id);
      if(tone == nullptr) return cli.Report(-2, NoToneExpl);
      tone->Output(*cli.obuf, 2, v);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The TSPORTS command.
//
class TsPortsCommand : public CliCommand
{
public:
   TsPortsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string TsPortsStr = "tsports";
fixed_string TsPortsExpl =
   "Displays the circuit(s) in a range of timeswitch ports.";

TsPortsCommand::TsPortsCommand() : CliCommand(TsPortsStr, TsPortsExpl)
{
   BindParm(*new PortMandParm);
   BindParm(*new PortOptParm);
}

fn_name TsPortsCommand_ProcessCommand = "TsPortsCommand.ProcessCommand";

word TsPortsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(TsPortsCommand_ProcessCommand);

   word first, last;
   bool one = false;

   if(!GetIntParm(first, cli)) return -1;

   switch(GetIntParmRc(last, cli))
   {
   case None:
      last = first;
      //  [[ fallthrough]]
   case Ok:
      break;
   default:
      return -1;
   }

   if(!cli.EndOfInput()) return -1;

   auto tsw = Singleton< Switch >::Instance();

   for(auto i = first; i <= last; ++i)
   {
      auto cct = tsw->GetCircuit(i);

      if(cct != nullptr)
      {
         one = true;
         *cli.obuf << spaces(2) << strIndex(i);

         if(first == last)
         {
            *cli.obuf << CRLF;
            cct->Output(*cli.obuf, 4, true);
         }
         else
         {
            *cli.obuf << "circuit : " << strObj(cct) << CRLF;
         }
      }
   }

   if(!one) return cli.Report(-2, NoCircuitsExpl);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The UNSUBSCRIBE command.
//
class UnsubscribeCommand : public CliCommand
{
public:
   UnsubscribeCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string UnsubscribeStr = "unsubscribe";
fixed_string UnsubscribeExpl = "Removes a feature from a DN.";

UnsubscribeCommand::UnsubscribeCommand() :
   CliCommand(UnsubscribeStr, UnsubscribeExpl)
{
   auto reg = Singleton< PotsFeatureRegistry >::Instance();
   BindParm(*new DnMandParm);
   BindParm(*reg->featuresUnsubscribe_);
}

fn_name UnsubscribeCommand_ProcessCommand = "UnsubscribeCommand.ProcessCommand";

word UnsubscribeCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(UnsubscribeCommand_ProcessCommand);

   word id1;
   id_t id2;

   if(!GetIntParm(id1, cli)) return -1;

   auto pro = Singleton< PotsProfileRegistry >::Instance()->Profile(id1);
   if(pro == nullptr) return cli.Report(-3, NotRegisteredExpl);

   if(!GetTextIndex(id2, cli)) return -1;
   if(!cli.EndOfInput()) return -1;

   if(!pro->Unsubscribe(id2)) return -4;
   return cli.Report(0, SuccessExpl);
}

//------------------------------------------------------------------------------
//
//  The POTS increment.
//
fixed_string PotsText = "pots";
fixed_string PotsExpl = "POTS Increment";

fn_name PotsIncrement_ctor = "PotsIncrement.ctor";

PotsIncrement::PotsIncrement() : CliIncrement(PotsText, PotsExpl)
{
   Debug::ft(PotsIncrement_ctor);

   BindCommand(*new TsPortsCommand);
   BindCommand(*new TonesCommand);
   BindCommand(*new MepsCommand);
   BindCommand(*new CodesCommand);
   BindCommand(*new DnsCommand);
   BindCommand(*new FeaturesCommand);
   BindCommand(*new RegisterCommand);
   BindCommand(*new DeregisterCommand);
   BindCommand(*new SubscribeCommand);
   BindCommand(*new ActivateCommand);
   BindCommand(*new DeactivateCommand);
   BindCommand(*new UnsubscribeCommand);
   BindCommand(*new ResetCommand);
   BindCommand(*new PbSizesCommand);
}

//------------------------------------------------------------------------------

fn_name PotsIncrement_dtor = "PotsIncrement.dtor";

PotsIncrement::~PotsIncrement()
{
   Debug::ftnt(PotsIncrement_dtor);
}
}
