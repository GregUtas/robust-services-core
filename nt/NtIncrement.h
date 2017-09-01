//==============================================================================
//
//  NtIncrement.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef NTINCREMENT_H_INCLUDED
#define NTINCREMENT_H_INCLUDED

#include "CliCommand.h"
#include <string>
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
   virtual ~CorruptWhatParm() { }
};

class CorruptCommand : public CliCommand
{
public:
   static const id_t PoolIndex = 1;
   static const id_t LastNtIndex = PoolIndex;

   //  Set BIND to false if binding a subclass of CorruptWhatParm.
   //
   explicit CorruptCommand(bool bind = true);
   virtual ~CorruptCommand() { }
protected:
   virtual word ProcessSubcommand(CliThread& cli, id_t index) const override;
private:
   virtual word ProcessCommand(CliThread& cli) const override;
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
   virtual ~NtSetWhatParm() { }
};

class NtSetCommand : public SetCommand
{
public:
   static const id_t FuncTraceScope = LastNbIndex + 1;
   static const id_t LastNtIndex    = FuncTraceScope;

   //  Set BIND to false if binding a subclass of NtSetWhatParm.
   //
   explicit NtSetCommand(bool bind = true);
   virtual ~NtSetCommand() { }
protected:
   virtual word ProcessSubcommand(CliThread& cli, id_t index) const override;
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
   virtual ~NtSaveWhatParm() { }
};

class NtSaveCommand : public SaveCommand
{
public:
   static const id_t FuncsIndex = LastNbIndex + 1;
   static const id_t LastNtIndex = FuncsIndex;

   //  Set BIND to false if binding a subclass of NtSaveWhatParm.
   //
   explicit NtSaveCommand(bool bind = true);
   virtual ~NtSaveCommand() { }
protected:
   virtual word ProcessSubcommand(CliThread& cli, id_t index) const override;
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
   virtual ~SizesCommand() { }
protected:
   virtual void DisplaySizes(CliThread& cli, bool all) const;
private:
   virtual word ProcessCommand(CliThread& cli) const override;
};

//------------------------------------------------------------------------------
//
//  Supports testcases.  Defined here so that other increments can subclass it.
//
class TestcaseAction : public CliTextParm
{
public:
   TestcaseAction();
   virtual ~TestcaseAction() { }
};

class TestcaseCommand : public CliCommand
{
public:
   static const id_t TestPrologIndex  = 1;
   static const id_t TestEpilogIndex  = 2;
   static const id_t TestRecoverIndex = 3;
   static const id_t TestBeginIndex   = 4;
   static const id_t TestEndIndex     = 5;
   static const id_t TestFailedIndex  = 6;
   static const id_t TestQueryIndex   = 7;
   static const id_t TestResetIndex   = 8;
   static const id_t LastNtIndex = TestResetIndex;

   //  Set BIND to false if binding a subclass of TestcaseAction.
   //
   explicit TestcaseCommand(bool bind = true);
   virtual ~TestcaseCommand() { }
protected:
   virtual word ProcessSubcommand(CliThread& cli, id_t index) const override;
   virtual void ConcludeTest(CliThread& cli) const;
   virtual void InitiateTest(CliThread& cli, const std::string& curr) const;
private:
   virtual word ProcessCommand(CliThread& cli) const override;
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
