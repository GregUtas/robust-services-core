//==============================================================================
//
//  Location.h
//
//  Diplomacy AI Client - Part of the DAIDE project (www.daide.org.uk).
//
//  (C) David Norman 2002 david@ellought.demon.co.uk
//  (C) Greg Utas 2019 greg@pentennea.com
//
//  This software may be reused for non-commercial purposes without charge,
//  and without notifying the authors.  Use of any part of this software for
//  commercial purposes without permission from the authors is prohibited.
//
#ifndef LOCATION_H_INCLUDED
#define LOCATION_H_INCLUDED

#include <iosfwd>
#include <set>
#include "DipTypes.h"
#include "Token.h"

//------------------------------------------------------------------------------

namespace Diplomacy
{
//  A province with a coast qualifier.  The coast is
//  o TOKEN_UNIT_AMY for an army
//  o TOKEN_UNIT_FLT for a fleet, if the province is not divided into coasts
//  o TOKEN_COAST_<dir> for a fleet, if the province is divided into coasts
//
//  Examples:
//  o Paris: only TOKEN_UNIT_AMY.
//  o Brest: only TOKEN_UNIT_AMY or TOKEN_UNIT_FLT.
//  o North Sea: only TOKEN_UNIT_FLT.
//  o Spain: only TOKEN_UNIT_AMY, TOKEN_COAST_NCS, or TOKEN_COAST_SCS.
//
struct Location
{
   //  The location's province.  Note that this is *not* a Token.
   //
   ProvinceId province;

   //  The coast within the province.
   //
   Token coast;

   //  Initializes members to default values.
   //
   Location();

   //  Creates the location for a specified province and coast.
   //
   Location(ProvinceId p, const Token& c);

   //  Constructs a location that arrived in a message.  Sets the
   //  coast so that it is correct for UNIT_TYPE.
   //
   Location(const TokenMessage& where, const Token& unit_type);

   //  Constructs UNIT's location from a message (e.g. from the tokens
   //  (TOKEN_UNIT_FLT (TOKEN_PROVINCE_STP TOKEN_COAST_NCS)).
   //
   Location(const TokenMessage& unit);

   //  Copy constructor.
   //
   Location(const Location& that) = default;

   //  Copy operator.
   //
   Location& operator=(const Location& that) = default;

   //  Used for sorting.
   //
   bool operator<(const Location& that) const;
};

//  Inserts a string for LOC into STREAM.  This should not be used to
//  build messages or display them verbatim.
//
std::ostream& operator<<(std::ostream& stream, const Location& loc);

//  For holding a set of locations.
//
typedef std::set< Location > LocationSet;
}
#endif
