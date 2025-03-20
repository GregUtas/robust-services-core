//==============================================================================
//
//  DipTypes.h
//
//  Diplomacy AI Client - Part of the DAIDE project (www.daide.org.uk).
//
//  (C) David Norman 2002 david@ellought.demon.co.uk
//  (C) Greg Utas 2019-2025 greg@pentennea.com
//
//  This software may be reused for non-commercial purposes without charge,
//  and without notifying the authors.  Use of any part of this software for
//  commercial purposes without permission from the authors is prohibited.
//
#ifndef DIPTYPES_H_INCLUDED
#define DIPTYPES_H_INCLUDED

#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <set>
#include "NwTypes.h"
#include "SysTypes.h"
#include "ToolTypes.h"

using namespace NetworkBase;
using namespace NodeBase;

//------------------------------------------------------------------------------
//
//  Types.
//
namespace Diplomacy
{
typedef int16_t ProvinceId;  // identifies a province
typedef int16_t PowerId;     // identifies a power
typedef uint16_t token_t;    // token's integral representation
typedef uint8_t category_t;  // token's category (MSB)
typedef uint8_t subtoken_t;  // token's value (LSB)

//  Constants.
//
constexpr int16_t POWER_MAX = 256;
constexpr int16_t NIL_POWER = -1;

constexpr int16_t PROVINCE_MAX = 256;
constexpr int16_t NIL_PROVINCE = -1;

constexpr token_t INVALID_TOKEN = 0xFFFF;
constexpr size_t NO_ERROR = SIZE_MAX;      // parsed message was error-free
constexpr int NIL_MOVE_NUMBER = -1;        // a move that hasn't been numbered

//  Basic collection types.
//
typedef std::list<ProvinceId> UnitList;
typedef std::set<ProvinceId> UnitSet;
typedef std::set<PowerId> PowerSet;
typedef std::set<ProvinceId> ProvinceSet;

//  Default server and client port numbers.
//
constexpr ipport_t ServerIpPort = 16713;
constexpr ipport_t ClientIpPort = 0xda1b;

//  Identifiers for classes derived from ones in NodeBase.
//
constexpr FlagId DipTracer = FirstAppTracer;

//  For sending and receiving Diplomacy messages.
//
class DipIpBuffer;

//  For managing a DipIpBuffer.
//
typedef std::unique_ptr<DipIpBuffer> DipIpBufferPtr;

//  For defining internal events.
//
typedef uint8_t BotEvent;
}
#endif
