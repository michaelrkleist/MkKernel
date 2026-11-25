////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Foundation\MkFoundationPCH.h"
#include "Process\MkCallStack.h"

// Mk includes.
#include "Process\MkCriticalSection.h"

// Windows includes.
#include "dbghelp.h"

#pragma comment(lib, "dbghelp.lib") 

// Static initializations.
bool sInitialized = false;
MkCriticalSectionLock sInitializedLock;
MkCriticalSectionLock sGetSymbolsLock;


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkCallStack
////////////////////////////////////////////////////////////////////////////////////////////////////

mkuint MkCallStack::Capture(
        std::vector<mkuint64>& addresses,
        mkuint32 skipCnt,
        mkuint32 levelCnt)
{
    {
        MkCriticalSection cs(sInitializedLock);
        if (!sInitialized)
        {
            Initialize();
            sInitialized = true;
        }
    }

    addresses.resize(levelCnt);
    mkuint32 addressCnt = ::RtlCaptureStackBackTrace(
                skipCnt, levelCnt, (PVOID*)&addresses[0], (PDWORD)nullptr);
    addresses.resize(addressCnt);

    return addressCnt;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkCallStack::GetStackInformation(
        const std::vector<mkuint64>& addresses,
        const bool getModuleName,
        std::vector<MkCallStack::StackInfo>& stackInfos)
{
    HANDLE processHandle = GetCurrentProcess();

    MkCriticalSection cs(sGetSymbolsLock);

    bool gotSymbols = false;

    stackInfos.clear();
    for (auto& addressIter : addresses)
    {
        MkCallStack::StackInfo stackInfo;

        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        DWORD dwOffset;

        if (!SymGetLineFromAddr64(processHandle, addressIter, &dwOffset, &line))
        {
            stackInfo.m_line = 0;
            stackInfo.m_address = addressIter;
            stackInfos.push_back(stackInfo);
            continue;
        }

        stackInfo.m_file = MkAString(line.FileName);
        stackInfo.m_line = line.LineNumber;
        stackInfo.m_address = addressIter;

        static mkuint64 symbol[
                (sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64)];
        PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbol;

        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 dwOffset64;
        if (!SymFromAddr(processHandle, addressIter, &dwOffset64, pSymbol))
        {
            DWORD error = GetLastError();
            stackInfos.push_back(stackInfo);
            continue;
        }

        stackInfo.m_symbol = MkAString(pSymbol->Name);
        gotSymbols = true;

        if (getModuleName)
        {
            IMAGEHLP_MODULE64 moduleInfo;
            ::memset(&moduleInfo, 0, sizeof(IMAGEHLP_MODULE64));
            moduleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

            bool hasModuleInfo = (SymGetModuleInfo64(processHandle, pSymbol->Address, &moduleInfo) == TRUE);

            if (!hasModuleInfo)
            {
                moduleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE64) - sizeof(BOOL) * 2;
                hasModuleInfo = (SymGetModuleInfo64(processHandle, pSymbol->Address, &moduleInfo) == TRUE);
            }

            if (hasModuleInfo)
            {
                stackInfo.m_module = MkAString(moduleInfo.ImageName);
            }
        }

        stackInfos.push_back(stackInfo);
    }

    return gotSymbols;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkCallStack::Initialize()
{
    HANDLE processHandle = GetCurrentProcess();

    SymSetOptions(
            SYMOPT_UNDNAME |
            SYMOPT_DEFERRED_LOADS | 
            SYMOPT_LOAD_LINES |
            SYMOPT_FAVOR_COMPRESSED | 
            SYMOPT_INCLUDE_32BIT_MODULES);

    CHAR executablePathName[MAX_PATH + 1];
    mkuint32 nameCharCnt = GetModuleFileNameA(NULL, executablePathName, MAX_PATH);
    while (nameCharCnt > 0 && executablePathName[--nameCharCnt] != '\\');
    executablePathName[nameCharCnt] = '\0';

    if (!::SymInitialize(processHandle, nameCharCnt > 0 ? executablePathName : NULL, TRUE))
    {
        DWORD errorCode = ::GetLastError();

        return;
    }

    ::atexit(Terminate);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkCallStack::Terminate()
{
    SymCleanup(GetCurrentProcess());
}