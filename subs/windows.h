//==============================================================================
//
//  windows.h
//
#ifndef WINDOWS_H_INCLUDED
#define WINDOWS_H_INCLUDED

#include "cstddef"
#include "cstdint"
#include "ctime"

//------------------------------------------------------------------------------
//
//  Windows basic stuff
//
typedef uint16_t WORD;
typedef uint32_t u_long;
typedef uint32_t DWORD;
typedef uint64_t DWORD64;
typedef void*    HANDLE;
typedef void*    LPVOID;

WORD    MAKEWORD(uint8_t a, uint8_t b);
uint8_t LOBYTE(WORD w);
uint8_t HIBYTE(WORD w);
DWORD   GetLastError();

const int ERROR_NOT_ENOUGH_MEMORY = 8;
const int ERROR_NOT_OWNER = 288;

//------------------------------------------------------------------------------
//
//  Windows time
//
union LARGE_INTEGER
{
   struct u
   {
      DWORD   LowPart;
      int32_t HighPart;
   };

   int64_t QuadPart;
};

bool QueryPerformanceFrequency(LARGE_INTEGER* Frequency);
bool QueryPerformanceCounter(LARGE_INTEGER* PerformanceCount);

typedef int errno_t;

errno_t localtime_s(tm* Tm, const time_t* Time);

//------------------------------------------------------------------------------
//
//  Windows heaps
//
typedef int32_t HRESULT;
const   HRESULT S_OK = 0;
typedef uint64_t SIZE_T;

HANDLE  GetProcessHeap();
DWORD   GetProcessHeaps(DWORD numberOfHeaps, HANDLE* processHeaps);
HANDLE  HeapCreate(DWORD opts, SIZE_T initialSize, SIZE_T maxSize);
HANDLE  HeapAlloc(HANDLE heap, DWORD flags, SIZE_T bytes);
bool    HeapValidate(HANDLE heap, DWORD flags, const void* mem);
bool    HeapFree(HANDLE heap, DWORD flags, void* mem);
bool    HeapDestroy(HANDLE heap);

//------------------------------------------------------------------------------
//
//  Windows synchronization
//
const uint32_t INFINITE = 0xffffffff;
const uint32_t WAIT_OBJECT_0 = 0;
const uint32_t WAIT_ABANDONED = 0x80;
const uint32_t WAIT_TIMEOUT = 258;

DWORD  WaitForSingleObject(HANDLE handle, DWORD msecs);
HANDLE CreateMutex(void* EventAttributes, bool initialOwner, const wchar_t* name);
bool   ReleaseMutex(HANDLE mutex);
bool   CloseHandle(HANDLE object);
HANDLE CreateEvent(void* EventAttributes, bool ManualReset, bool InitialState, const wchar_t* Name);
bool   SetEvent(HANDLE Event);

//------------------------------------------------------------------------------
//
//  Windows threads
//
const int32_t THREAD_PRIORITY_BELOW_NORMAL = -1;
const int32_t THREAD_PRIORITY_NORMAL = 0;
const int32_t THREAD_PRIORITY_ABOVE_NORMAL = 1;
const int32_t THREAD_PRIORITY_HIGHEST = 2;

const DWORD DUPLICATE_SAME_ACCESS = 0x00000002;
const DWORD HIGH_PRIORITY_CLASS = 0x00000080;

HANDLE  GetCurrentProcess();
bool    DuplicateHandle(HANDLE SourceProcessHandle, HANDLE SourceHandle,
                        HANDLE TargetProcessHandle, HANDLE* TargetHandle,
                        DWORD DesiredAccess, bool InheritHandle, DWORD Options);
bool    SetPriorityClass(HANDLE Process, DWORD PriorityClass);

typedef DWORD (*LPTHREAD_START_ROUTINE)(void* ThreadParameter);
HANDLE  CreateThread(void* ThreadAttributes, SIZE_T StackSize, LPTHREAD_START_ROUTINE StartAddress,
                     void* ThreadParameter, DWORD CreationFlags, DWORD* ThreadId);
HANDLE  GetCurrentThread();
DWORD   GetCurrentThreadId();
bool    SetThreadPriority(HANDLE Thread, int Priority);

//------------------------------------------------------------------------------
//
//  Windows structured exceptions
//
const uint32_t DBG_CONTROL_C                   = 0x40010005;
const uint32_t DBG_CONTROL_BREAK               = 0x40010008;
const uint32_t STATUS_DATATYPE_MISALIGNMENT    = 0x80000002;
const uint32_t STATUS_ACCESS_VIOLATION         = 0xC0000005;
const uint32_t STATUS_IN_PAGE_ERROR            = 0xC0000006;
const uint32_t STATUS_INVALID_HANDLE           = 0xC0000008;
const uint32_t STATUS_NO_MEMORY                = 0xC0000017;
const uint32_t STATUS_ILLEGAL_INSTRUCTION      = 0xC000001D;
const uint32_t STATUS_NONCONTINUABLE_EXCEPTION = 0xC0000025;
const uint32_t STATUS_INVALID_DISPOSITION      = 0xC0000026;
const uint32_t STATUS_ARRAY_BOUNDS_EXCEEDED    = 0xC000008C;
const uint32_t STATUS_FLOAT_DENORMAL_OPERAND   = 0xC000008D;
const uint32_t STATUS_FLOAT_DIVIDE_BY_ZERO     = 0xC000008E;
const uint32_t STATUS_FLOAT_INEXACT_RESULT     = 0xC000008F;
const uint32_t STATUS_FLOAT_INVALID_OPERATION  = 0xC0000090;
const uint32_t STATUS_FLOAT_OVERFLOW           = 0xC0000091;
const uint32_t STATUS_FLOAT_STACK_CHECK        = 0xC0000092;
const uint32_t STATUS_FLOAT_UNDERFLOW          = 0xC0000093;
const uint32_t STATUS_INTEGER_DIVIDE_BY_ZERO   = 0xC0000094;
const uint32_t STATUS_INTEGER_OVERFLOW         = 0xC0000095;
const uint32_t STATUS_PRIVILEGED_INSTRUCTION   = 0xC0000096;
const uint32_t STATUS_STACK_OVERFLOW           = 0xC00000FD;

typedef void (*_se_translator_function)(uint32_t ErrVal, void* ExceptionPointers);
_se_translator_function _set_se_translator(_se_translator_function NewPtFunc);
int _resetstkoflw();

#endif