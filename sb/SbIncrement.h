//==============================================================================
//
//  SbIncrement.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SBINCREMENT_H_INCLUDED
#define SBINCREMENT_H_INCLUDED

#include "CliIncrement.h"
#include "NbTypes.h"
#include "NwIncrement.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  SessionBase additions to the Clear command.
//
class SbClearWhatParm : public NwClearWhatParm
{
public:
   SbClearWhatParm();
   virtual ~SbClearWhatParm() { }
};

class SbClearCommand : public NwClearCommand
{
public:
   static const id_t FactoryIndex   = LastNwIndex + 1;
   static const id_t FactoriesIndex = LastNwIndex + 2;
   static const id_t ProtocolIndex  = LastNwIndex + 3;
   static const id_t ProtocolsIndex = LastNwIndex + 4;
   static const id_t SignalIndex    = LastNwIndex + 5;
   static const id_t SignalsIndex   = LastNwIndex + 6;
   static const id_t ServiceIndex   = LastNwIndex + 7;
   static const id_t ServicesIndex  = LastNwIndex + 8;
   static const id_t TimersIndex    = LastNwIndex + 9;
   static const id_t LastSbIndex    = LastNwIndex + 9;

   //  Set BIND to false if binding a subclass of NwClearWhatParm.
   //
   explicit SbClearCommand(bool bind = true);
   virtual ~SbClearCommand() { }
protected:
   virtual word ProcessSubcommand(CliThread& cli, id_t index) const override;
};

//------------------------------------------------------------------------------
//
//  SessionBase additions to the Exclude command.
//
class SbExcludeWhatParm : public NwExcludeWhatParm
{
public:
   SbExcludeWhatParm();
   virtual ~SbExcludeWhatParm() { }
};

class SbExcludeCommand : public NwExcludeCommand
{
public:
   static const id_t FactoryIndex  = LastNwIndex + 1;
   static const id_t ProtocolIndex = LastNwIndex + 2;
   static const id_t SignalIndex   = LastNwIndex + 3;
   static const id_t ServiceIndex  = LastNwIndex + 4;
   static const id_t TimersIndex   = LastNwIndex + 5;
   static const id_t LastSbIndex   = LastNwIndex + 6;

   //  Set BIND to false if binding a subclass of SbExcludeWhatParm.
   //
   explicit SbExcludeCommand(bool bind = true);
   virtual ~SbExcludeCommand() { }
protected:
   virtual word ProcessSubcommand(CliThread& cli, id_t index) const override;
};

//------------------------------------------------------------------------------
//
//  SessionBase additions to the Include command.
//
class SbIncludeWhatParm : public NwIncludeWhatParm
{
public:
   SbIncludeWhatParm();
   virtual ~SbIncludeWhatParm() { }
};

class SbIncludeCommand : public NwIncludeCommand
{
public:
   static const id_t FactoryIndex  = LastNwIndex + 1;
   static const id_t ProtocolIndex = LastNwIndex + 2;
   static const id_t SignalIndex   = LastNwIndex + 3;
   static const id_t ServiceIndex  = LastNwIndex + 4;
   static const id_t TimersIndex   = LastNwIndex + 5;
   static const id_t LastSbIndex   = LastNwIndex + 5;

   //  Set BIND to false if binding a subclass of SbIncludeWhatParm.
   //
   explicit SbIncludeCommand(bool bind = true);
   virtual ~SbIncludeCommand() { }
protected:
   virtual word ProcessSubcommand(CliThread& cli, id_t index) const override;
};

//------------------------------------------------------------------------------
//
//  SessionBase additions to the Query command.
//
class SbQueryCommand : public NwQueryCommand
{
public:
   //  Set BIND to false if binding a subclass of QueryWhatParm.
   //
   explicit SbQueryCommand(bool bind = true);
   virtual ~SbQueryCommand() { }
protected:
   virtual word ProcessSubcommand(CliThread& cli, id_t index) const override;
};

//------------------------------------------------------------------------------
//
//  SessionBase additions to the Status command.
//
class SbStatusCommand : public NwStatusCommand
{
public:
   SbStatusCommand() { }
   virtual ~SbStatusCommand() { }
private:
   virtual word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------
//
//  The increment that provides commands for SessionBase.
//
class SbIncrement : public CliIncrement
{
   friend class Singleton< SbIncrement >;
private:
   //  Private because this singleton is not subclassed.
   //
   SbIncrement();

   //  Private because this singleton is not subclassed.
   //
   ~SbIncrement();
};
}
#endif
