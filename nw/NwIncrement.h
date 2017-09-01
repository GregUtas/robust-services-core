//==============================================================================
//
//  NwIncrement.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef NWINCREMENT_H_INCLUDED
#define NWINCREMENT_H_INCLUDED

#include "CliIncrement.h"
#include "NbIncrement.h"
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Network layer additions to the Clear command.
//
class NwClearWhatParm : public ClearWhatParm
{
public:
   NwClearWhatParm();
   virtual ~NwClearWhatParm() { }
};

class NwClearCommand : public ClearCommand
{
public:
   static const id_t PeerIndex   = LastNbIndex + 1;
   static const id_t PeersIndex  = LastNbIndex + 2;
   static const id_t PortIndex   = LastNbIndex + 3;
   static const id_t PortsIndex  = LastNbIndex + 4;
   static const id_t LastNwIndex = LastNbIndex + 4;

   //  Set BIND to false if binding a subclass of NwClearWhatParm.
   //
   explicit NwClearCommand(bool bind = true);
   virtual ~NwClearCommand() { }
protected:
   virtual word ProcessSubcommand(CliThread& cli, id_t index) const override;
};

//------------------------------------------------------------------------------
//
//  Network layer additions to the Exclude command.
//
class NwExcludeWhatParm : public ExcludeWhatParm
{
public:
   NwExcludeWhatParm();
   virtual ~NwExcludeWhatParm() { }
};

class NwExcludeCommand : public ExcludeCommand
{
public:
   static const id_t ExcludePeerIndex = LastNbIndex + 1;
   static const id_t ExcludePortIndex = LastNbIndex + 2;
   static const id_t LastNwIndex      = LastNbIndex + 2;

   //  Set BIND to false if binding a subclass of NwExcludeWhatParm
   //
   explicit NwExcludeCommand(bool bind = true);
   virtual ~NwExcludeCommand() { }
protected:
   virtual word ProcessSubcommand(CliThread& cli, id_t index) const override;
};

//------------------------------------------------------------------------------
//
//  Network layer additions to the Include command.
//
class NwIncludeWhatParm : public IncludeWhatParm
{
public:
   NwIncludeWhatParm();
   virtual ~NwIncludeWhatParm() { }
};

class NwIncludeCommand : public IncludeCommand
{
public:
   static const id_t IncludePeerIndex = LastNbIndex + 1;
   static const id_t IncludePortIndex = LastNbIndex + 2;
   static const id_t LastNwIndex      = LastNbIndex + 2;

   //  Set BIND to false if binding a subclass of NwIncludeWhatParm.
   //
   explicit NwIncludeCommand(bool bind = true);
   virtual ~NwIncludeCommand() { }
protected:
   virtual word ProcessSubcommand(CliThread& cli, id_t index) const override;
};

//------------------------------------------------------------------------------
//
//  Network layer additions to the Query command.
//
class NwQueryCommand : public QueryCommand
{
public:
   //  Set BIND to false if binding a subclass of QueryWhatParm.
   //
   explicit NwQueryCommand(bool bind = true);
   virtual ~NwQueryCommand() { }
protected:
   virtual word ProcessSubcommand(CliThread& cli, id_t index) const override;
};

//------------------------------------------------------------------------------
//
//  Network layer additions to the Status command.
//
class NwStatusCommand : public StatusCommand
{
public:
   NwStatusCommand() { }
   virtual ~NwStatusCommand() { }
protected:
   virtual word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------
//
//  The increment that provides commands for the Network layer.
//
class NwIncrement : public CliIncrement
{
   friend class Singleton< NwIncrement >;
private:
   //  Private because this singleton is not subclassed.
   //
   NwIncrement();

   //  Private because this singleton is not subclassed.
   //
   ~NwIncrement();
};
}
#endif
