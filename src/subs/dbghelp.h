//==============================================================================
//
//  dbghelp.h
//
#ifdef OS_WIN
#ifndef DBGHELP_H_INCLUDED
#define DBGHELP_H_INCLUDED

#include "cstddef"
#include "windows.h"

//------------------------------------------------------------------------------
//
//  Windows thread stacks
//
constexpr size_t MAX_SYM_NAME = 2000;

struct SYMBOL_INFO
{
   size_t SizeOfStruct;
   size_t MaxNameLen;
   char*  Name;
};

struct IMAGEHLP_LINE64
{
   size_t SizeOfStruct;
   char*  FileName;
   size_t LineNumber;
};

constexpr DWORD SYMOPT_UNDNAME = 0x00000002;
constexpr DWORD SYMOPT_LOAD_LINES = 0x00000010;

WORD  RtlCaptureStackBackTrace(DWORD FramesToSkip, DWORD FramesToCapture, void* BackTrace, DWORD* BackTraceHash);
bool  SymInitialize(HANDLE Process, const char* UserSearchPath, bool InvadeProcess);
DWORD SymGetOptions();
DWORD SymSetOptions(DWORD SymOptions);
bool  SymFromAddr(HANDLE Process, DWORD64 Address, DWORD64* Displacement, SYMBOL_INFO* Symbol);
bool  SymGetLineFromAddr64(HANDLE Process, DWORD64 Addr, DWORD* Displacement, IMAGEHLP_LINE64* Line64);
bool  SymCleanup(HANDLE Process);

#endif
#endif
