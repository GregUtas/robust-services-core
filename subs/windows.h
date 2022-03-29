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
typedef uint32_t DWORD;
typedef uint64_t DWORD64;
typedef unsigned long ULONG;
typedef unsigned long u_long;
typedef void* HANDLE;

WORD    MAKEWORD(uint8_t a, uint8_t b);
uint8_t LOBYTE(WORD w);
uint8_t HIBYTE(WORD w);
DWORD   GetLastError();

constexpr int ERROR_NOT_ENOUGH_MEMORY = 0x0008;
constexpr int ERROR_NOT_SUPPORTED = 0x0032;
constexpr int ERROR_INVALID_PARAMETER = 0x0057;
constexpr int ERROR_NOT_OWNER = 0x0120;

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
//  Windows memory
//
typedef uint64_t SIZE_T;

constexpr DWORD PAGE_NOACCESS = 0x01;
constexpr DWORD PAGE_READONLY = 0x02;
constexpr DWORD PAGE_READWRITE = 0x04;
constexpr DWORD PAGE_EXECUTE = 0x10;
constexpr DWORD PAGE_EXECUTE_READ = 0x20;
constexpr DWORD PAGE_EXECUTE_READWRITE = 0x40;

constexpr DWORD MEM_COMMIT = 0x1000;
constexpr DWORD MEM_RESERVE = 0x2000;
constexpr DWORD MEM_RELEASE = 0x8000;

void* VirtualAlloc(void* addr, SIZE_T size, DWORD allocType, DWORD prot);
bool VirtualFree(void* addr, SIZE_T size, DWORD freeType);
bool VirtualLock(void* addr, SIZE_T size);
bool VirtualUnlock(void* addr, SIZE_T size);
bool VirtualProtect(void* addr, SIZE_T size, DWORD newProt, DWORD* oldProt);

//------------------------------------------------------------------------------
//
//  Windows heaps
//
constexpr DWORD HEAP_GENERATE_EXCEPTIONS = 0x00000004;

typedef int32_t HRESULT;
constexpr HRESULT S_OK = 0;

HANDLE  GetProcessHeap();
DWORD   GetProcessHeaps(DWORD numberOfHeaps, HANDLE* processHeaps);
HANDLE  HeapCreate(DWORD opts, SIZE_T initialSize, SIZE_T maxSize);
HANDLE  HeapAlloc(HANDLE heap, DWORD flags, SIZE_T bytes);
SIZE_T  HeapSize(HANDLE heap, DWORD flags, const void* mem);
bool    HeapValidate(HANDLE heap, DWORD flags, const void* mem);
bool    HeapFree(HANDLE heap, DWORD flags, void* mem);
bool    HeapDestroy(HANDLE heap);

//------------------------------------------------------------------------------
//
//  Windows synchronization
//
constexpr uint32_t INFINITE = 0xffffffff;
constexpr uint32_t WAIT_OBJECT_0 = 0;
constexpr uint32_t WAIT_ABANDONED = 0x80;
constexpr uint32_t WAIT_TIMEOUT = 258;

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
constexpr int32_t THREAD_PRIORITY_BELOW_NORMAL = -1;
constexpr int32_t THREAD_PRIORITY_NORMAL = 0;
constexpr int32_t THREAD_PRIORITY_ABOVE_NORMAL = 1;
constexpr int32_t THREAD_PRIORITY_HIGHEST = 2;

constexpr DWORD DUPLICATE_SAME_ACCESS = 0x00000002;
constexpr DWORD HIGH_PRIORITY_CLASS = 0x00000080;

HANDLE  GetCurrentProcess();
bool    DuplicateHandle(HANDLE SourceProcessHandle, HANDLE SourceHandle,
                        HANDLE TargetProcessHandle, HANDLE* TargetHandle,
                        DWORD DesiredAccess, bool InheritHandle, DWORD Options);
bool    SetPriorityClass(HANDLE Process, DWORD PriorityClass);

HANDLE  GetCurrentThread();
DWORD   GetCurrentThreadId();
bool    SetThreadPriority(HANDLE Thread, int Priority);
bool    SetThreadPriorityBoost(HANDLE Thread, bool disable);

//------------------------------------------------------------------------------
//
//  Windows processes
//
struct STARTUPINFOA
{
   DWORD cb;
   char* lpReserved;
   char* lpDesktop;
   char* lpTitle;
   DWORD dwX;
   DWORD dwY;
   DWORD dwXSize;
   DWORD dwYSize;
   DWORD dwXCountChars;
   DWORD dwYCountChars;
   DWORD dwFillAttribute;
   DWORD dwFlags;
   WORD wShowWindow;
   WORD cbReserved2;
   unsigned char* lpReserved2;
   HANDLE hStdInput;
   HANDLE hStdOutput;
   HANDLE hStdError;
};

struct PROCESS_INFORMATION
{
   HANDLE hProcess;
   HANDLE hThread;
   DWORD  dwProcessId;
   DWORD  dwThreadId;
};

bool CreateProcessA(
   const char* lpApplicationName,
   char* lpCommandLine,
   void* lpProcessAttributes,
   void* lpThreadAttributes,
   bool bInheritHandles,
   DWORD dwCreationFlags,
   void* lpEnvironment,
   const char* lpCurrentDirectory,
   STARTUPINFOA* lpStartupInfo,
   PROCESS_INFORMATION* lpProcessInformation);

void GetExitCodeProcess(HANDLE hProcess, DWORD* lpExitCode);

//------------------------------------------------------------------------------
//
//  Windows structured exceptions
//
constexpr uint32_t DBG_CONTROL_C                   = 0x40010005;
constexpr uint32_t DBG_CONTROL_BREAK               = 0x40010008;
constexpr uint32_t STATUS_DATATYPE_MISALIGNMENT    = 0x80000002;
constexpr uint32_t STATUS_ACCESS_VIOLATION         = 0xC0000005;
constexpr uint32_t EXCEPTION_ACCESS_VIOLATION      = 0xC0000005;
constexpr uint32_t STATUS_IN_PAGE_ERROR            = 0xC0000006;
constexpr uint32_t STATUS_INVALID_HANDLE           = 0xC0000008;
constexpr uint32_t STATUS_NO_MEMORY                = 0xC0000017;
constexpr uint32_t STATUS_ILLEGAL_INSTRUCTION      = 0xC000001D;
constexpr uint32_t STATUS_NONCONTINUABLE_EXCEPTION = 0xC0000025;
constexpr uint32_t STATUS_INVALID_DISPOSITION      = 0xC0000026;
constexpr uint32_t STATUS_ARRAY_BOUNDS_EXCEEDED    = 0xC000008C;
constexpr uint32_t STATUS_FLOAT_DENORMAL_OPERAND   = 0xC000008D;
constexpr uint32_t STATUS_FLOAT_DIVIDE_BY_ZERO     = 0xC000008E;
constexpr uint32_t STATUS_FLOAT_INEXACT_RESULT     = 0xC000008F;
constexpr uint32_t STATUS_FLOAT_INVALID_OPERATION  = 0xC0000090;
constexpr uint32_t STATUS_FLOAT_OVERFLOW           = 0xC0000091;
constexpr uint32_t STATUS_FLOAT_STACK_CHECK        = 0xC0000092;
constexpr uint32_t STATUS_FLOAT_UNDERFLOW          = 0xC0000093;
constexpr uint32_t STATUS_INTEGER_DIVIDE_BY_ZERO   = 0xC0000094;
constexpr uint32_t STATUS_INTEGER_OVERFLOW         = 0xC0000095;
constexpr uint32_t STATUS_PRIVILEGED_INSTRUCTION   = 0xC0000096;
constexpr uint32_t STATUS_STACK_OVERFLOW           = 0xC00000FD;

struct EXCEPTION_RECORD
{
   DWORD ExceptionCode;
   DWORD NumberParameters;
   uintptr_t ExceptionInformation[15];
};

struct _EXCEPTION_POINTERS
{
   EXCEPTION_RECORD* ExceptionRecord;
};

typedef void (*_se_translator_function)(uint32_t ErrVal, _EXCEPTION_POINTERS* ex);
_se_translator_function _set_se_translator(_se_translator_function NewPtFunc);
int _resetstkoflw();

//------------------------------------------------------------------------------
//
//  Windows console
//
constexpr int SW_MINIMIZE = 6;
constexpr int SW_RESTORE = 9;

HANDLE GetConsoleWindow();
bool ShowWindow(HANDLE window, int mode);
bool SetConsoleTitle(const wchar_t* title);
#endif