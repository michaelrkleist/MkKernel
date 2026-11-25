////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFoundation.h"
#include "Basic\MkString.h"

// Windows includes.
#include <windows.h>


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkCallStack
////////////////////////////////////////////////////////////////////////////////////////////////////

class MK_EXPORT_FOUNDATION MkCallStack
{
public:
    struct MK_EXPORT_FOUNDATION StackInfo
    {
        const MkAString& GetFile() const
        { return m_file; }

        mkuint GetLine() const
        { return m_line; }

        const MkAString& GetSymbol() const
        { return m_symbol; }

        mkuint64 GetAddress() const
        { return m_address; }

        const MkAString& GetModule() const
        { return m_module; }

        MkAString m_file;
        MkAString m_symbol;
        MkAString m_module;
        mkuint m_line = 0;
        mkuint64 m_address = 0;
    };

    static mkuint Capture(
            std::vector<mkuint64>& addresses,
            mkuint32 skipCnt,
            mkuint32 levelCnt);

    static bool GetStackInformation(
            const std::vector<mkuint64>& addresses,
            const bool getModuleName,
            std::vector<StackInfo>& stackInfos);

private:
    MkCallStack();
    MkCallStack(const MkCallStack& other);

    static void Initialize();
    static void Terminate();
};


