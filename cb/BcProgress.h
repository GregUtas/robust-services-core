//==============================================================================
//
//  BcProgress.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef BCPROGRESS_H_INCLUDED
#define BCPROGRESS_H_INCLUDED

#include "TlvIntParameter.h"
#include <cstdint>
#include <iosfwd>
#include <string>
#include "SbTypes.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace CallBase
{
//  Progress indicators.
//
namespace Progress
{
   //  Type for progress indicators.
   //
   typedef uint8_t Ind;

   constexpr Ind NilInd         = 0; // default value
   constexpr Ind EndOfSelection = 1; // facility for incoming call chosen
   constexpr Ind Alerting       = 2; // facility acknowledged incoming call
   constexpr Ind Suspend        = 3; // will clear call if timer expires
   constexpr Ind Resume         = 4; // resumed call before timer expired
   constexpr Ind MediaUpdate    = 5; // sending media from a new address
   constexpr Ind MaxInd         = 5; // range constant

   //  Returns a string for displaying ID.
   //
   const char* strInd(Ind ind);
}

//------------------------------------------------------------------------------
//
//  Progress indicator parameter.
//
struct ProgressInfo
{
public:
   //  Constructs the nil instance.
   //
   ProgressInfo();

   //  Displays member variables, similar to Base::Display.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;

   //  The progress indicator.
   //
   Progress::Ind progress;
};

//------------------------------------------------------------------------------
//
//  Virtual base class for supporting a ProgressInfo parameter.
//
class ProgressParameter : public TlvIntParameter< Progress::Ind >
{
protected:
   //  Protected because this class is virtual.
   //
   ProgressParameter(ProtocolId prid, Id pid);

   //  Protected because subclasses should be singletons.
   //
   virtual ~ProgressParameter();

   //  Overridden to invoke Info::Display.
   //
   virtual void DisplayMsg(std::ostream& stream, const std::string& prefix,
      const byte_t* bytes, size_t count) const override;

   //  Overridden to create a CLI parameter for ProgressInfo.
   //
   virtual CliParm* CreateCliParm(Usage use) const override;
};
}
#endif
