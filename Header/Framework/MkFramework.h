////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFoundation.h"

// Limit what is included when windows.h is included.
#define WIN32_LEAN_AND_MEAN
//#include <windows.h>

// DLL support.
// Normally import the DLL symbols.
#define MK_EXPORT_FRAMEWORK __declspec(dllimport)

// Except when building the particular DLL.
#ifdef MK_BUILD_FRAMEWORK
#undef MK_EXPORT_FRAMEWORK
#define MK_EXPORT_FRAMEWORK  __declspec(dllexport)
#endif