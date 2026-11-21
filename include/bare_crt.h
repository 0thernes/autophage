#ifndef AUTOPHAGE_BARE_CRT_H
#define AUTOPHAGE_BARE_CRT_H

/*
 * AUTOPHAGE Bare-Metal Minimal CRT & OS Portability Layer
 *
 * Allows zero-dependency compilation with:
 *   1. Bundled LLVM/Clang targeting x86_64-w64-windows-gnu (even without MinGW libc headers!)
 *   2. Standard GCC / Clang on Linux x86_64
 *   3. MSVC / full MinGW toolchains
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#if defined(_WIN32) && defined(AUTOPHAGE_BARE_WIN64)

typedef struct _iobuf FILE;
extern FILE* __cdecl __acrt_iob_func(unsigned index);
#define stdin  (__acrt_iob_func(0))
#define stdout (__acrt_iob_func(1))
#define stderr (__acrt_iob_func(2))

extern int    __cdecl printf(const char* fmt, ...);
extern int    __cdecl fprintf(FILE* stream, const char* fmt, ...);
extern int    __cdecl snprintf(char* buf, size_t n, const char* fmt, ...);
extern int    __cdecl putchar(int c);
extern int    __cdecl fgetc(FILE* stream);
extern int    __cdecl fflush(FILE* stream);
extern FILE*  __cdecl fopen(const char* filename, const char* mode);
extern size_t __cdecl fwrite(const void* ptr, size_t size, size_t count, FILE* stream);
extern size_t __cdecl fread(void* ptr, size_t size, size_t count, FILE* stream);
extern char*  __cdecl fgets(char* str, int num, FILE* stream);
extern int    __cdecl fclose(FILE* stream);
extern int    __cdecl fseek(FILE* stream, long offset, int origin);
extern int    __cdecl remove(const char* filename);
extern int    __cdecl setvbuf(FILE* stream, char* buf, int mode, size_t size);
#define EOF      (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define _IONBF 0x0004
#define _IOLBF 0x0040

extern void*  __cdecl malloc(size_t size);
extern void*  __cdecl calloc(size_t num, size_t size);
extern void*  __cdecl realloc(void* ptr, size_t size);
extern void   __cdecl free(void* ptr);
extern void   __cdecl exit(int status);
extern int    __cdecl atoi(const char* str);
extern long   __cdecl strtol(const char* str, char** endptr, int base);
extern unsigned long __cdecl strtoul(const char* str, char** endptr, int base);
extern unsigned long long __cdecl strtoull(const char* str, char** endptr, int base);
extern double __cdecl strtod(const char* str, char** endptr);

extern void*  __cdecl memcpy(void* dst, const void* src, size_t n);
extern void*  __cdecl memmove(void* dst, const void* src, size_t n);
extern void*  __cdecl memset(void* dst, int c, size_t n);
extern int    __cdecl memcmp(const void* a, const void* b, size_t n);
extern size_t __cdecl strlen(const char* s);
extern int    __cdecl strcmp(const char* a, const char* b);
extern int    __cdecl strncmp(const char* a, const char* b, size_t n);
extern char*  __cdecl strstr(const char* hay, const char* needle);

extern double __cdecl log(double x);
extern double __cdecl log2(double x);
extern double __cdecl exp(double x);
extern double __cdecl pow(double x, double y);
extern double __cdecl sqrt(double x);
extern double __cdecl fabs(double x);
extern double __cdecl sin(double x);
extern double __cdecl cos(double x);

/* Minimal Win32 Kernel32 bare-metal bindings (no giant windows.h needed) */
typedef void* HANDLE;
typedef uint32_t DWORD;
typedef int BOOL;
typedef uint32_t (__stdcall *LPTHREAD_START_ROUTINE)(void* lpParameter);

typedef struct _EXCEPTION_RECORD {
    DWORD ExceptionCode;
    DWORD ExceptionFlags;
    struct _EXCEPTION_RECORD* ExceptionRecord;
    void* ExceptionAddress;
    DWORD NumberParameters;
    uintptr_t ExceptionInformation[15];
} EXCEPTION_RECORD;

typedef struct _EXCEPTION_POINTERS {
    EXCEPTION_RECORD* ExceptionRecord;
    void* ContextRecord;
} EXCEPTION_POINTERS;

typedef long (__stdcall *PVECTORED_EXCEPTION_HANDLER)(EXCEPTION_POINTERS* ExceptionInfo);

#define MEM_COMMIT             0x00001000u
#define MEM_RESERVE            0x00002000u
#define MEM_RELEASE            0x00008000u
#define PAGE_READWRITE         0x04u
#define PAGE_EXECUTE_READ      0x20u
#define PAGE_EXECUTE_READWRITE 0x40u
#define INFINITE               0xFFFFFFFFu

__declspec(dllimport) void*  __stdcall VirtualAlloc(void* lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect);
__declspec(dllimport) BOOL   __stdcall VirtualFree(void* lpAddress, size_t dwSize, DWORD dwFreeType);
__declspec(dllimport) BOOL   __stdcall VirtualProtect(void* lpAddress, size_t dwSize, DWORD flNewProtect, DWORD* lpflOldProtect);
__declspec(dllimport) BOOL   __stdcall FlushInstructionCache(HANDLE hProcess, const void* lpBaseAddress, size_t dwSize);
__declspec(dllimport) HANDLE __stdcall GetCurrentProcess(void);
__declspec(dllimport) HANDLE __stdcall CreateThread(void* lpAttr, size_t dwStackSize, LPTHREAD_START_ROUTINE lpStart, void* lpParam, DWORD dwFlags, DWORD* lpThreadId);
__declspec(dllimport) DWORD  __stdcall WaitForMultipleObjects(DWORD nCount, const HANDLE* lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);
__declspec(dllimport) BOOL   __stdcall CloseHandle(HANDLE hObject);
__declspec(dllimport) BOOL   __stdcall QueryPerformanceCounter(int64_t* lpPerformanceCount);
__declspec(dllimport) BOOL   __stdcall QueryPerformanceFrequency(int64_t* lpFrequency);
__declspec(dllimport) void   __stdcall Sleep(DWORD dwMilliseconds);
__declspec(dllimport) void*  __stdcall AddVectoredExceptionHandler(uint32_t First, PVECTORED_EXCEPTION_HANDLER Handler);
__declspec(dllimport) uint32_t __stdcall RemoveVectoredExceptionHandler(void* Handle);
__declspec(dllimport) HANDLE __stdcall GetStdHandle(DWORD nStdHandle);
__declspec(dllimport) BOOL   __stdcall PeekNamedPipe(HANDLE hNamedPipe, void* lpBuf, DWORD nBufSize, DWORD* lpBytesRead, DWORD* lpTotalBytesAvail, DWORD* lpBytesLeft);
#define STD_INPUT_HANDLE ((DWORD)-10)

#else
/* Standard POSIX / Full MinGW / MSVC headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#endif

#endif

/* Portable finite-FP64 check (works identically under bare Win64 & standard libc) */
static inline int ap_isfinite(double x) {
    return (x == x) && ((x - x) == 0.0);
}

#endif /* AUTOPHAGE_BARE_CRT_H */
