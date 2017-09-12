//==============================================================================
//
//  SysFile.win.cpp
//
//  Copyright (C) 2012-2014 Greg Utas.  All rights reserved.
//
#include "SysFile.h"
#include <cstdio>
#include "Debug.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

const string SysFile_Startup = "SysSignals.Startup";

void SysFile::Startup()
{
   Debug::ft(&SysFile_Startup);

   //  By default, Windows allows a maximum of 512 open file handles.  This
   //  causes problems for CodeTools, which currently keeps every code file
   //  open.  Eventually it should be changed to reopen files as required,
   //  but for now...
   //
   _setmaxstdio(1024);
}