//==============================================================================
//
//  NtIncrement.h
//
//  Copyright (C) 2013-2020  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later
//  version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef NTINCREMENT_H_INCLUDED
#define NTINCREMENT_H_INCLUDED

#include "CliCommand.h"
#include "CliIncrement.h"
#include "CliTextParm.h"
#include "NbIncrement.h"
#include "NbTypes.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace NodeTools
{
//  Corrupts a critical data structure in order to test error recovery.
//  Defined here so that other increments can subclass it.
//
class CorruptWhatParm : public CliTextParm
{
public:
   CorruptWhatParm();
   virtual ~CorruptWhatParm() = default;
};

class CorruptCommand : public CliCommand
{
public:
   static const id_t PoolIndex = 1;
   static const id_t LastNtIndex = PoolIndex;

   //  Set BIND to false if binding a subclass of CorruptWhatParm.
   //
   explicit CorruptCommand(bool bind = true);
   virtual ~CorruptCommand() = default;
protected:
   word ProcessSubcommand(CliThread& cli, id_t index) const override;
private:
   word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------
//
//  Configures trace tools.  Defined here so that other increments can
//  subclass it.
//
class NtSetWhatParm : public SetWhatParm
{
public:
   NtSetWhatParm();
   virtual ~NtSetWhatParm() = default;
};

class NtSetCommand : public SetCommand
{
public:
   static const id_t FuncTraceScope = LastNbIndex + 1;
   static const id_t LastNtIndex = FuncTraceScope;

   //  Set BIND to false if binding a subclass of NtSetWhatParm.
   //
   explicit NtSetCommand(bool bind = true);
   virtual ~NtSetCommand() = default;
protected:
   word ProcessSubcommand(CliThread& cli, id_t index) const override;
};

//------------------------------------------------------------------------------
//
//  Saves data captured by trace tools in a file.  Defined here so that other
//  increments can subclass it.
//
class NtSaveWhatParm : public SaveWhatParm
{
public:
   NtSaveWhatParm();
   virtual ~NtSaveWhatParm() = default;
};

class NtSaveCommand : public SaveCommand
{
public:
   static const id_t FuncsIndex = LastNbIndex + 1;
   static const id_t LastNtIndex = FuncsIndex;

   //  Set BIND to false if binding a subclass of NtSaveWhatParm.
   //
   explicit NtSaveCommand(bool bind = true);
   virtual ~NtSaveCommand() = default;
protected:
   word ProcessSubcommand(CliThread& cli, id_t index) const override;
};

//------------------------------------------------------------------------------
//
//  Displays the size of classes and structs defined by a module's layer.
//  Defined here so that other increments can subclass it.
//
class SizesCommand : public CliCommand
{
public:
   SizesCommand();
   virtual ~SizesCommand() = default;
protected:
   virtual void DisplaySizes(CliThread& cli, bool all) const;
private:
   word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------
//
//  Supports tests.  Defined here so that other increments can subclass it.
//
class TestsAction : public CliTextParm
{
public:
   TestsAction();
   virtual ~TestsAction() = default;
};

class TestsCommand : public CliCommand
{
public:
   static const id_t TestPrologIndex = 1;
   static const id_t TestEpilogIndex = 2;
   static const id_t TestRecoverIndex = 3;
   static const id_t TestBeginIndex = 4;
   static const id_t TestEndIndex = 5;
   static const id_t TestFailedIndex = 6;
   static const id_t TestQueryIndex = 7;
   static const id_t TestRetestIndex = 8;
   static const id_t TestEraseIndex = 9;
   static const id_t TestResetIndex = 10;
   static const id_t LastNtIndex = TestResetIndex;

   //  Set BIND to false if binding a subclass of TestsAction.
   //
   explicit TestsCommand(bool bind = true);
   virtual ~TestsCommand() = default;
protected:
   word ProcessSubcommand(CliThread& cli, id_t index) const override;
private:
   word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------
//
//  Increment for NodeBase tools and tests.
//
class NtIncrement : public CliIncrement
{
   friend class Singleton< NtIncrement >;
private:
   //  Private because this singleton is not subclassed.
   //
   NtIncrement();

   //  Private because this singleton is not subclassed.
   //
   ~NtIncrement();
};
}
#endif
