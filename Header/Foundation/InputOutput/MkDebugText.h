////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFoundation.h"
#include "Basic\MkString.h"
#include "Basic\MkTime.h"
#include "Process\MkThread.h"

// Windows includes.
#include <windows.h>

// C++ includes.
#include <iostream>
#include <sstream>

// Forward declarations.


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkDebugText
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkDebugText
{
public:
    MkDebugText() = default;

    virtual ~MkDebugText() 
    {
#if 0
        OutputDebugStringA(m_output.str().c_str());
#else
        std::cout << m_output.str().c_str();
#endif
    }

    template<typename T>
    std::ostringstream& operator<<(const T& value) 
    { m_output << value; return m_output; }

    std::ostringstream& operator<<(const MkAString& str)
    { m_output << str.GetCString(); return m_output; }

protected:
    std::ostringstream m_output;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkDebugSystemText
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkDebugSystemText : public MkDebugText
{
public:
    MkDebugSystemText()
    { 
        m_output << MkCPUTime().GetAString(MkDuration::eSeconds).GetCString() << ' ';
        m_output << MkThread::GetCurrentThreadId() << ' ';
    }

    ~MkDebugSystemText() override = default;
};