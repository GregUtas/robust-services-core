//==============================================================================
//
//  ConvoySubversion.h
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
#ifndef CONVOYSUBVERSION_H_INCLUDED
#define CONVOYSUBVERSION_H_INCLUDED

#include <cstdint>
#include "DipTypes.h"

//------------------------------------------------------------------------------

namespace Diplomacy
{
//  A convoying army "subverts" another convoy when the army's attack would
//    (a) cut a support for a convoying fleet
//    (b) cut a support for an attack on a convoying fleet
//  Subversions need to be resolved differently than regular attacks because
//  they can create paradoxical situations.  A simple example is
//    o Turkey: Smy-Gre VIA Aeg, Aeg C Smy-Gre
//    o Austria: Gre S TURKISH Aeg
//    o Italy: Ion-Aeg, Eas S Ion-Aeg
//  If the convoy succeeds, it cuts the support for its fleet, which means
//  that its fleet is dislodged, which means that the convoy fails and its
//  army does not cut the support, which means that the convoy succeeds...
//
enum SubversionType : uint8_t
{
   UNSUBVERTED_CONVOY,  // convoy is not subverted by another convoying army
   SUBVERTED_CONVOY,    // convoy is subverted by another convoying army
   CONFUSED_CONVOY      // convoy is a member of a paradoxical loop
};

struct ConvoySubversion
{
   ProvinceId subverted_army;       // the army whose convoy this one subverts
   int8_t number_of_subversions;    // how many convoys subvert this one
   SubversionType subversion_type;  // how this convoy was subverted

   ConvoySubversion();  // sets members to default values
   void decrement();    // removes one subversion
   void clear();        // removes all subversions
};
}
#endif
