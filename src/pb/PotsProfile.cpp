//==============================================================================
//
//  PotsProfile.cpp
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
#include "PotsProfile.h"
#include <bitset>
#include <cstdint>
#include <sstream>
#include <string>
#include "Algorithms.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "MsgPort.h"
#include "NbTypes.h"
#include "PotsCliParms.h"
#include "PotsFeatureProfile.h"
#include "PotsFeatureRegistry.h"
#include "PotsProfileRegistry.h"
#include "ProtocolSM.h"
#include "Restart.h"
#include "RootServiceSM.h"
#include "SbAppIds.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
PotsProfile::PotsProfile(DN dn)
{
   Debug::ft("PotsProfile.ctor");

   //  Set the profile's identifier to its DN, create a circuit for it,
   //  initialize its queue of subscribed features, and add it to the
   //  POTS profile registry.
   //
   dn_.SetId(Address::DNToIndex(dn));
   circuit_.reset(new PotsCircuit(*this));
   featureq_.Init(PotsFeatureProfile::LinkDiff());
   dyn_.reset(new PotsProfileDynamic);

   Singleton< PotsProfileRegistry >::Instance()->BindProfile(*this);
}

//------------------------------------------------------------------------------

PotsProfile::~PotsProfile()
{
   Debug::ftnt("PotsProfile.dtor");

   //  Remove the profile from the registry.
   //
   Singleton< PotsProfileRegistry >::Extant()->UnbindProfile(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t PotsProfile::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast< const PotsProfile* >(&local);
   return ptrdiff(&fake->dn_, fake);
}

//------------------------------------------------------------------------------

bool PotsProfile::ClearObjAddr(const LocalAddress& addr)
{
   Debug::ftnt("PotsProfile.ClearObjAddr(addr)");

   //  For purposes of error recovery, transition to the idle state
   //  if the address is unknown.
   //
   if((addr == LocalAddress()) || (dyn_->objAddr_ == addr))
   {
      dyn_->objAddr_ = LocalAddress();
      if(dyn_->state_ == Active) dyn_->state_ = Idle;
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

bool PotsProfile::ClearObjAddr(const ProtocolSM* psm)
{
   Debug::ftnt("PotsProfile.ClearObjAddr(psm)");

   if(psm == nullptr) return false;
   auto port = psm->Port();
   if(port != nullptr) return ClearObjAddr(port->ObjAddr());
   return false;
}

//------------------------------------------------------------------------------

bool PotsProfile::Deregister()
{
   Debug::ft("PotsProfile.Deregister");

   FunctionGuard guard(Guard_MemUnprotect);

   for(auto f = featureq_.First(); f != nullptr; f = featureq_.First())
   {
      if(!Unsubscribe(f->Fid())) return false;
   }

   delete this;
   return true;
}

//------------------------------------------------------------------------------

void PotsProfile::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Address::Display(stream, prefix, options);

   stream << prefix << "DN       : " << Address::IndexToDN(GetDN());
   stream << prefix << "DN id    : " << dn_.to_str() << CRLF;
   stream << prefix << "state    : " << dyn_->state_ << CRLF;
   stream << prefix << "objAddr  : " << dyn_->objAddr_.to_str() << CRLF;
   stream << prefix << "circuit  : ";

   if(options.test(DispVerbose))
   {
      if(circuit_ != nullptr)
      {
         stream << CRLF;
         circuit_->Display(stream, prefix + spaces(2), options);
      }
      else
      {
         stream << "unassigned" << CRLF;
      }
   }
   else
   {
      stream << strObj(circuit_.get()) << CRLF;
   }

   stream << prefix << "featureq : " << CRLF;
   featureq_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

PotsFeatureProfile* PotsProfile::FindFeature(PotsFeature::Id fid) const
{
   Debug::ft("PotsProfile.FindFeature");

   for(auto f = featureq_.First(); f != nullptr; featureq_.Next(f))
   {
      if(f->Fid() == fid) return f;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

bool PotsProfile::HasFeature(PotsFeature::Id fid) const
{
   Debug::ft("PotsProfile.HasFeature");

   return (FindFeature(fid) != nullptr);
}

//------------------------------------------------------------------------------

bool PotsProfile::SetObjAddr(const MsgPort& port)
{
   Debug::ft("PotsProfile.SetObjAddr");

   //  Fail if PORT is invalid.
   //
   if(MsgPort::Find(port.ObjAddr()) == nullptr) return false;

   //  Overwrite the profile's current port if it is invalid.
   //
   if(MsgPort::Find(dyn_->objAddr_) != nullptr)
   {
      //  The current port is valid.  If the new port's root SSM is a
      //  multiplexer, it has created a user-side PSM, so overwrite the
      //  current port.  A multiplexer is inserted between an existing
      //  call and the POTS circuit, taking over communication with the
      //  circuit.
      //
      auto root = port.RootSsm();

      if((root == nullptr) || (root->Sid() != PotsMuxServiceId))
      {
         return false;
      }
   }

   dyn_->objAddr_ = port.ObjAddr();
   if(dyn_->state_ == Idle) dyn_->state_ = Active;
   return true;
}

//------------------------------------------------------------------------------

void PotsProfile::SetState(const ProtocolSM* psm, State state)
{
   Debug::ft("PotsProfile.SetState");

   if(psm == nullptr) return;
   auto port = psm->Port();
   if(port == nullptr) return;

   if(port->ObjAddr() == dyn_->objAddr_)
   {
      dyn_->state_ = state;
   }
}

//------------------------------------------------------------------------------

void PotsProfile::Shutdown(RestartLevel level)
{
   Debug::ft("PotsProfile.Shutdown");

   if(Restart::ClearsMemory(MemType())) return;

   //  If the circuit will be freed, reset the data related to it.
   //
   FunctionGuard guard(Guard_MemUnprotect);
   Restart::Release(circuit_);
   if(circuit_ == nullptr) new (dyn_.get()) PotsProfileDynamic();
}

//------------------------------------------------------------------------------

void PotsProfile::Startup(RestartLevel level)
{
   Debug::ft("PotsProfile.Startup");

   if(circuit_ == nullptr)
   {
      FunctionGuard guard(Guard_MemUnprotect);
      circuit_.reset(new PotsCircuit(*this));
   }
}

//------------------------------------------------------------------------------

bool PotsProfile::Subscribe(PotsFeature::Id fid, CliThread& cli)
{
   Debug::ft("PotsProfile.Subscribe");

   auto reg = Singleton< PotsFeatureRegistry >::Instance();

   for(auto fp = featureq_.First(); fp != nullptr; featureq_.Next(fp))
   {
      auto ftr = reg->Feature(fp->Fid());

      if(ftr->IsIncompatible(fid))
      {
         *cli.obuf << spaces(2) << IncompatibleFeature;
         *cli.obuf << ftr->AbbrName() << '.' << CRLF;
         return false;
      }
   }

   auto ftr = reg->Feature(fid);

   if(ftr != nullptr)
   {
      FunctionGuard guard(Guard_MemUnprotect);

      auto fp = ftr->Subscribe(*this, cli);

      if(fp != nullptr)
      {
         featureq_.Enq(*fp);
         return true;
      }
   }
   else
   {
      *cli.obuf << spaces(2) << FeatureNotInstalled << CRLF;
   }

   return false;
}

//------------------------------------------------------------------------------

bool PotsProfile::Unsubscribe(PotsFeature::Id fid)
{
   Debug::ft("PotsProfile.Unsubscribe");
   for(auto fp = featureq_.First(); fp != nullptr; featureq_.Next(fp))
   {
      if(fp->Fid() == fid)
      {
         FunctionGuard guard(Guard_MemUnprotect);

         if(!fp->Unsubscribe(*this)) return false;
         featureq_.Exq(*fp);
         delete fp;
         return true;
      }
   }

   return false;
}
}
