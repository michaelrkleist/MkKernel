////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Foundation\MkFoundationPCH.h"
#include "Foundation\Basic\MkPolymorphicClass.h"

// Mk includes.
#include "Process\MkCriticalSection.h"

// NOTE: Should probably do something stronger to enforce proper initialization of
// sClassMap.  MRK.
#pragma warning(disable : 4073)
#pragma init_seg(lib)

// Static initializations.
namespace
{
std::map<MkAString, void* (*)()> sClassMap;
MkCriticalSectionLock sClassMapLock;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkPolymorphicClass
////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkPolymorphicClass::Register(const MkAString& className, void* (*Create)())
{
    MkCriticalSection cs(sClassMapLock);
    sClassMap.insert(std::make_pair(className, Create));
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void* MkPolymorphicClass::CreateInstance(const MkAString& className)
{
    auto classIter = sClassMap.find(className);
    
    if (classIter == sClassMap.end()) return nullptr;

    // Exercise that Create static method.
    return (classIter->second)();
}
