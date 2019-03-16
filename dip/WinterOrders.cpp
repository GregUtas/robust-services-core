//==============================================================================
//
//  WinterOrders.cpp
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
#include "WinterOrders.h"
#include <iterator>
#include <ostream>
#include <utility>
#include "Location.h"
#include "SysTypes.h"
#include "Token.h"

using std::ostream;

//------------------------------------------------------------------------------

namespace Diplomacy
{
WinterOrders::WinterOrders() :
   number_of_orders_required(0),
   number_of_waives(0),
   is_building(false)
{
}

//------------------------------------------------------------------------------

size_t WinterOrders::get_number_of_results() const
{
   return (adjustments.size() + number_of_waives);
}

//------------------------------------------------------------------------------

ostream& operator<<(ostream& stream, const WinterOrders& orders)
{
   if(orders.adjustments.size() + orders.number_of_waives == 0)
   {
      stream << "none";
      return stream;
   }

   if(!orders.adjustments.empty())
   {
      stream << (orders.is_building ? "Build" : "Remove");
   }

   for(auto a = orders.adjustments.begin(); a != orders.adjustments.cend(); ++a)
   {
      stream << SPACE << a->first.coast << SPACE << a->first;
      if(std::next(a) != orders.adjustments.cend()) stream << ',';
   }

   if(orders.number_of_waives > 0)
   {
      if(!orders.adjustments.empty()) stream << ", ";
      stream << "Waives " << orders.number_of_waives;
   }

   return stream;
}
}
