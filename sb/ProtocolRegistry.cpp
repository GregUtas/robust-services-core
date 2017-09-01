//==============================================================================
//
//  ProtocolRegistry.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "ProtocolRegistry.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "Protocol.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name ProtocolRegistry_ctor = "ProtocolRegistry.ctor";

ProtocolRegistry::ProtocolRegistry()
{
   Debug::ft(ProtocolRegistry_ctor);

   protocols_.Init(Protocol::MaxId + 1, Protocol::CellDiff(), MemProt);
}

//------------------------------------------------------------------------------

fn_name ProtocolRegistry_dtor = "ProtocolRegistry.dtor";

ProtocolRegistry::~ProtocolRegistry()
{
   Debug::ft(ProtocolRegistry_dtor);
}

//------------------------------------------------------------------------------

fn_name ProtocolRegistry_BindProtocol = "ProtocolRegistry.BindProtocol";

bool ProtocolRegistry::BindProtocol(Protocol& protocol)
{
   Debug::ft(ProtocolRegistry_BindProtocol);

   return protocols_.Insert(protocol);
}

//------------------------------------------------------------------------------

void ProtocolRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "protocols [ProtocolId]" << CRLF;
   protocols_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

Protocol* ProtocolRegistry::GetProtocol(ProtocolId prid) const
{
   return protocols_.At(prid);
}

//------------------------------------------------------------------------------

void ProtocolRegistry::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name ProtocolRegistry_UnbindProtocol = "ProtocolRegistry.UnbindProtocol";

void ProtocolRegistry::UnbindProtocol(Protocol& protocol)
{
   Debug::ft(ProtocolRegistry_UnbindProtocol);

   protocols_.Erase(protocol);
}
}
