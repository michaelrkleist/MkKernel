////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFoundation.h"

enum ExceptionType
{
    eOSFailure = 0,
    eOSConfiguration = 0,
    eLogicError
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkException
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkException
{
public:
    MkException(ExceptionType exceptionType, const char* pCause) :
        m_exceptionType(exceptionType),
        m_pCause(pCause)
    { }
    ~MkException() = default;

    ExceptionType GetExceptionType() const
    { return m_exceptionType; }

    const char* GetCauseString() const
    { return m_pCause; }

    const char* GetTypeString() const;

private:
    ExceptionType m_exceptionType;
    const char* m_pCause;
};


