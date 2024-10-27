#pragma once

#define CONFIG_H_IS_INCLUDED

#define IN_USE 1
#define NOT_IN_USE 0
#define USING(X) (X & X)


#ifdef __linux__
#define LINUX_OS
#elif defined(_WIN32) // #ifdef __linux__
#define WINDOWS_OS
#elif defined(__ANDROID__)
#define ANDROID_OS
#endif // #elif defined(WIN32) // #ifdef __linux__


#ifndef NDEBUG
#define RELEASE_BUILD NOT_IN_USE
#else
#define RELEASE_BUILD IN_USE
#endif

#ifndef DEBUG
#if !USING(RELEASE_BUILD)
#error Either NDEBUG or DEBUG must be defined!
#endif  // #if !USING(RELEASE_BUILD)
#define DEBUG_BUILD NOT_IN_USE
#else
#if USING(RELEASE_BUILD)
#error DEBUG is defined on a RELEASE BUILD
#endif  // #if USING(RELEASE_BUILD)
#define DEBUG_BUILD IN_USE
#endif  // #if DEBUG

////////////////////////////////////////////////////////////////////////////////
// PE build is intended for injecting bytecode into a template VM binary
#ifdef PE_BUILD
////////////////////////////////////////////////////////////////////////////////
#undef PE_BUILD
#define PE_BUILD IN_USE
// #pragma optimize("gsy",on)
// #pragma comment(linker,"/merge:.rdata=.data")
// #pragma comment(linker,"/merge:.text=.data")
// #pragma comment(linker,"/merge:.reloc=.data")
#ifdef WINDOWS
#pragma comment(linker, "/OPT:NOWIN98")
#endif  // #ifdef WINDOWS
////////////////////////////////////////////////////////////////////////////////
#else   // #if USING(PE_BUILD )
////////////////////////////////////////////////////////////////////////////////
#undef PE_BUILD
#define PE_BUILD NOT_IN_USE
#endif  // #else//#endif //#ifdef PE_BUILD
static_assert(!USING(PE_BUILD) || !USING(DEBUG_BUILD));
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define MAX_U8_COUNT (1L << 8)

#define MACHINE_BITS 8
#define MAX_OPCODE_BITS MACHINE_BITS
#define MAX_OPCODE_VALUE ((1L << MAX_OPCODE_BITS) - 1)
