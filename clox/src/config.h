#pragma once

#define CONFIG_H_IS_INCLUDED

#define IN_USE      1
#define NOT_IN_USE  0
#define USING(X)    ( X & X )

#ifndef NDEBUG
#define RELEASE_BUILD NOT_IN_USE
#else
#define RELEASE_BUILD IN_USE
#endif

#ifndef DEBUG
#define DEBUG_BUILD !USING(RELEASE_BUILD)
#else
#if USING(RELEASE_BUILD)
#error DEBUG is defined on a RELEASE BUILD
#endif // #if USING(RELEASE_BUILD)
#define DEBUG_BUILD IN_USE
#endif // #if DEBUG
