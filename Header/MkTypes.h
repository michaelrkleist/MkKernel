///////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
///////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// C++ includes.
#include <stdint.h>

// The platform is assumed to be a native 64 bit platform.  MRK.

// Unambiguous types suitable for persistence.  Used for binary serialization just because.
typedef unsigned __int32 mkbool32;  // Kind of aggravating but alternatives are worse.
typedef unsigned char mkbyte;

typedef char mkchar8;
typedef wchar_t mkchar16;

typedef int16_t mkint16;
typedef int32_t mkint32;
typedef int64_t mkint64;

typedef uint16_t mkuint16;
typedef uint32_t mkuint32;
typedef uint64_t mkuint64;

typedef float mkfloat32;
typedef double mkfloat64;

// Native types that can be reassigned for 32 bit platforms.
typedef bool mkbool;
typedef int64_t mkint;
typedef uint64_t mkuint;    // mkuint is assumed to be sufficient for size_t in the kernel.
typedef double mkfloat;
typedef double mkdouble;

// NOTE: A while back I attempted to turn mkuint into a class so that size_t did not
// throw warnings.  It almost worked.  Kinda.  MRK.
// NOTE: Please don't get me started on size_t.  My blood pressure is bad enough.  MRK.
