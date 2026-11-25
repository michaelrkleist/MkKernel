////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFoundation.h"
#include "Basic\MkDuration.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkTime
////////////////////////////////////////////////////////////////////////////////////////////////////

// An absolute time measured from the epoch (which in this case is January 1, 1601).
class MK_EXPORT_FOUNDATION MkTime : public MkDuration
{
public:
    // Default initialization is now.
    MkTime();

    MkTime(const mkuint64 duration) :
        MkDuration(duration)
    { }

    MkTime(
            mkuint year,                // Four digit year
            mkuint month,               // Month 1..12
            mkuint day,                 // Month Day 1..31
            mkuint hour,                // Hour of 24 hour day
            mkuint minute = 0,
            mkuint second = 0,
            mkuint microSecond = 0);

    ~MkTime() override = default;

    static MkTime BeginningOfTime()
    { return MkTime(1ll); }

    static MkTime EndOfTime()
    { return MkTime(9'000'000'000'000'000'000ll); }
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkCPUTime
////////////////////////////////////////////////////////////////////////////////////////////////////

class MK_EXPORT_FOUNDATION MkCPUTime : public MkDuration
{
// A relative time measured from when the system started.
// NOTE: This value is less expensive to capture so it it useful for measuring relative
// times.  MRK.
public:
    // Default initialization is now.
    MkCPUTime();

    MkCPUTime(const mkint64 duration) :
        MkDuration(duration)
    { }

    MkCPUTime(const MkDuration& duration) :
        MkDuration(duration)
    { }

    ~MkCPUTime() override = default;

    static MkCPUTime BeginningOfTime()
    { return MkCPUTime(1ll); }

    static MkCPUTime EndOfTime()
    { return MkCPUTime(9'000'000'000'000'000'000ll); }
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkCPUTimer
////////////////////////////////////////////////////////////////////////////////////////////////////

class MK_EXPORT_FOUNDATION MkCPUTimer : public MkDuration
{
// An activity timer.  The timer starts at construction.                 
public:
    MkCPUTimer();
    ~MkCPUTimer() override = default;

    // Get time duration from the timer start until now.
    MkDuration GetDuration() const;

private:
    mkint64 m_startCPUTicks;
};
