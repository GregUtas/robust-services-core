//==============================================================================
//
//  WinterOrders.h
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
#ifndef WINTERORDERS_H_INCLUDED
#define WINTERORDERS_H_INCLUDED

#include <cstddef>
#include <iosfwd>
#include <map>

namespace Diplomacy
{
   class Token;
   struct Location;
}

//------------------------------------------------------------------------------

namespace Diplomacy
{
//  For holding an adjustment order.  The Location key is interpreted as an
//  order (e.g. TOKEN_PROVINCE_STP/TOKEN_UNIT_NCS builds a fleet in StP(nc)).
//  The Token indicates whether the order succeeded.
//
typedef std::map<Location, Token> Adjustments;

//  Adjustment orders.
//
struct WinterOrders
{
   Adjustments adjustments;           // units and results
   size_t number_of_orders_required;  // number of adjustments to be made
   size_t number_of_waives;           // number of waived adjustments
   bool is_building;                  // whether building or disbanding

   //  Initializes members to default values.
   //
   WinterOrders();

   //  Returns the number of builds or removals, plus waives.
   //
   size_t get_number_of_results() const;
};

//  Inserts a string that summarizes ORDERS into STREAM.
//
std::ostream& operator<<(std::ostream& stream, const WinterOrders& orders);
}
#endif
