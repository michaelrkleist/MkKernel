////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Foundation\MkFoundationPCH.h"
#include "Basic\MkTime.h"

// C++ includes.
#include <chrono>

// Windows includes.
#include <windows.h>

// Static initializations.
namespace
{
mkuint64 sCPUTicksPerSecond;
BOOL sCPUTicksPerSecondExists = QueryPerformanceFrequency((LARGE_INTEGER*)&sCPUTicksPerSecond);
mkfloat64 sFileTimePerCPUTick = 10'000'000.0 / (mkfloat64)sCPUTicksPerSecond;

// NOTE: A little sneaky.  MRK.
// NOTE: And why I need to do this with mkfloat64, I do not know.  MRK.
const MkDuration sBeginningOfTime(1.0, MkDuration::e100Nanoseconds);
const MkDuration sEndOfTime(18'000'000'000'000'000'000.0, MkDuration::e100Nanoseconds);

// Value for converting std::chrono::system_clock to MkTime.
const mkuint64 sUNIXEpochFileTime = 116444736000000000ull;

const mkuint64 sChronoEpochFileTime = sUNIXEpochFileTime -
        (std::chrono::nanoseconds(
                std::chrono::system_clock::from_time_t(time_t(0)) -
                        std::chrono::time_point<std::chrono::system_clock>{}).count() / 100);

MkDuration sSystemTimeOffset = MkDuration(0ll, MkDuration::eSeconds);

const mkuint sMonthDayCnt[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const mkuint FebuaryDayCount(const mkuint year)
{
    if (year % 4 == 0)
    {
        if (year % 100 == 0) return (year % 400 == 0) ? 29 : 28;
        else return 29;
    }
    return 28;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkTime
////////////////////////////////////////////////////////////////////////////////////////////////////

MkTime::MkTime() :
    MkDuration()
{
    // Convert a std::chrono::system_time into a filetime.
    m_duration = sChronoEpochFileTime +
            (std::chrono::nanoseconds(
                    std::chrono::system_clock::now() -
                            std::chrono::time_point<std::chrono::system_clock>{}).count() / 100);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkTime::MkTime(
        mkuint year,
        mkuint month,
        mkuint day,
        mkuint hour,
        mkuint minute,
        mkuint second,
        mkuint microSecond) :
    MkDuration()
{
    // Condition the values.
    while (month < 1)
    {
        --year;
        month += 12;
    }

    while (month > 12)
    {
        ++year;
        month -= 12;
    }

    mkint64 fileTimeDelta = 0;
    mkuint monthDayCnt = sMonthDayCnt[month];

    if (day < 1)
    {
        fileTimeDelta -= (1ll - day) * 24ll * 60ll * 60ll * 10000000ll;
        day = 1;
    }

    if (day > monthDayCnt)
    {
        if (month == 2)
        {
            monthDayCnt = FebuaryDayCount(year);
        }

        fileTimeDelta += ((mkint64)day - (mkint64)(monthDayCnt)) * 24ll * 60ll * 60ll * 10000000ll;
        day = monthDayCnt;
    }

    if (hour < 0)
    {
        fileTimeDelta -= (0ll - mkint64(hour)) * 60ll * 60ll * 10000000ll;
        hour = 0;
    }

    if (hour > 23)
    {
        fileTimeDelta += (mkint64(hour) - 23ll) * 60ll * 60ll * 10000000ll;
        hour = 23;
    }

    if (minute < 0)
    {
        fileTimeDelta -= (0ll - mkint64(minute)) * 60ll * 10000000ll;
        minute = 0;
    }

    if (minute > 59)
    {
        fileTimeDelta += (mkint64(minute) - 59ll) * 60ll * 10000000ll;
        minute = 59;
    }

    if (second < 0)
    {
        fileTimeDelta -= (0ll - mkint64(second)) * 10000000ll;
        second = 0;
    }

    if (second > 59)
    {
        fileTimeDelta += (mkint64(second) - 59ll) * 10000000ll;
        second = 59;
    }

    SYSTEMTIME utcTime;
    utcTime.wYear = year;
    utcTime.wMonth = month;
    utcTime.wDay = day;
    utcTime.wHour = hour;
    utcTime.wMinute = minute;
    utcTime.wSecond = second;
    utcTime.wMilliseconds = 0;

    utcTime.wDayOfWeek = 0;

    FILETIME fileTime;
    if (!::SystemTimeToFileTime(&utcTime, &fileTime)) return;

    m_duration = mkint64(fileTime.dwHighDateTime) << 32 | mkint64(fileTime.dwLowDateTime);
    m_duration += microSecond * 10;

    m_duration += fileTimeDelta;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkCPUTime
////////////////////////////////////////////////////////////////////////////////////////////////////

MkCPUTime::MkCPUTime()
{
    mkint64 cpuTicks;
    QueryPerformanceCounter((LARGE_INTEGER*)&cpuTicks);
    m_duration = (mkfloat64)cpuTicks * sFileTimePerCPUTick;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkCPUTimer
////////////////////////////////////////////////////////////////////////////////////////////////////

MkCPUTimer::MkCPUTimer()
{
    QueryPerformanceCounter((LARGE_INTEGER*)&m_startCPUTicks);
    m_duration = (mkfloat64)m_startCPUTicks * sFileTimePerCPUTick;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkDuration MkCPUTimer::GetDuration() const
{
    mkint64 cpuTicks;
    QueryPerformanceCounter((LARGE_INTEGER*)&cpuTicks);
    return MkDuration(
            (mkfloat64)(cpuTicks - m_startCPUTicks) * sFileTimePerCPUTick, MkDuration::e100Nanoseconds);
}