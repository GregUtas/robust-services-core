//==============================================================================
//
//  ConvoySubversion.cpp
//
//  Diplomacy AI Client - Part of the DAIDE project (www.daide.org.uk).
//
//  (C) David Norman 2002 david@ellought.demon.co.uk
//  (C) Greg Utas 2019-2022 greg@pentennea.com
//
//  This software may be reused for non-commercial purposes without charge,
//  and without notifying the authors.  Use of any part of this software for
//  commercial purposes without permission from the authors is prohibited.
//
#include "ConvoySubversion.h"

//------------------------------------------------------------------------------

namespace Diplomacy
{
ConvoySubversion::ConvoySubversion() :
   subverted_army(NIL_PROVINCE),
   number_of_subversions(0),
   subversion_type(UNSUBVERTED_CONVOY)
{
}

//------------------------------------------------------------------------------

void ConvoySubversion::clear()  // <b>
{
   number_of_subversions = 0;
   subversion_type = UNSUBVERTED_CONVOY;
}

//------------------------------------------------------------------------------

void ConvoySubversion::decrement()
{
   if(--number_of_subversions == 0)
   {
      subversion_type = UNSUBVERTED_CONVOY;
   }
}
}
