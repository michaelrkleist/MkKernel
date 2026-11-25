////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#pragma warning(disable : 4244)

// Mk includes.
#include "MkFoundation.h"
#include "Basic\MkString.h"

// NOTE: I looked into using std::chrono::duration.  What a ****ing mess.  By
// attempting to be (just about) everything to everbody, the committee made the
// 99.9% use case a collosal pain in the ass.  (See std::chrono::duration_cast
// amongst other transgressions against software engineering principles.)


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkDuration
////////////////////////////////////////////////////////////////////////////////////////////////////

// More specifically, a time duration.  The unit is 100 nanoseconds chosen to match
// Windows file times.
class MK_EXPORT_FOUNDATION MkDuration
{
public:
    enum TimeUnits
    {
        eNanoseconds = 0,
        e100Nanoseconds,
        eMicroseconds,
        eMilliseconds,
        eSeconds,
        eMinutes,
        eHours,
        eDays,
        eWeeks,
        eUnitCnt
    };

    MkDuration() :
        m_duration(0)
    { }

    MkDuration(const mkint64 unitCnt, const TimeUnits units);
    MkDuration(const mkdouble unitScalar, const TimeUnits units);

    virtual ~MkDuration() = default;

    bool operator==(const MkDuration& rhs) const
    { return m_duration == rhs.m_duration; }

    bool operator!=(const MkDuration& rhs) const
    { return m_duration != rhs.m_duration; }

    bool operator>(const MkDuration& rhs) const
    { return m_duration > rhs.m_duration; }

    bool operator>=(const MkDuration& rhs) const
    { return m_duration >= rhs.m_duration; }

    bool operator<(const MkDuration& rhs) const
    { return m_duration < rhs.m_duration; }

    bool operator<=(const MkDuration& rhs) const
    { return m_duration <= rhs.m_duration; }

    MkDuration operator+(const MkDuration& rhs) const
    { return MkDuration(m_duration + rhs.m_duration); }

    const MkDuration& operator+=(const MkDuration& rhs)
    { m_duration += rhs.m_duration; return *this; }

    MkDuration operator-(const MkDuration& rhs) const
    { return MkDuration(m_duration - rhs.m_duration); }

    const MkDuration& operator-=(const MkDuration& rhs)
    { m_duration -= rhs.m_duration; return *this; }

    MkDuration operator*(const mkdouble scalar) const
    { return MkDuration(m_duration * scalar); }

    const MkDuration& operator*=(const mkdouble scalar)
    { m_duration *= scalar; return *this; }

    MkDuration operator/(const mkdouble scalar) const
    { return MkDuration(m_duration / scalar); }

    const MkDuration& operator/=(const mkdouble scalar)
    { m_duration /= scalar; return *this; }

    mkint64 GetDuration() const
    { return m_duration; }

    mkfloat64 GetScalar(const TimeUnits units) const;
    mkint64 GetIntegral(const TimeUnits units) const;

    void SetToForever();
    bool IsForever() const;

    bool IsZero() const
    { return m_duration == 0; }

    bool IsPositive() const
    { return bool(m_duration > 0); }

    MkAString GetAString(const TimeUnits units) const;

protected:
    MkDuration(const mkint64 duration) :
        m_duration(duration)
    { }

    mkint64 m_duration;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Utility Inlines
////////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE: Is this better or worse than a strained constructor?  MRK.
inline MkDuration MkForever()
{ MkDuration forever; forever.SetToForever(); return forever; }

////////////////////////////////////////////////////////////////////////////////////////////////////

inline mkint64 Milliseconds(const MkDuration& duration)
{ return duration.GetIntegral(MkDuration::eMilliseconds); }

////////////////////////////////////////////////////////////////////////////////////////////////////

inline mkint64 Seconds(const MkDuration& duration)
{ return duration.GetIntegral(MkDuration::eSeconds); }

////////////////////////////////////////////////////////////////////////////////////////////////////

inline mkint64 Minutes(const MkDuration& duration)
{ return duration.GetIntegral(MkDuration::eMinutes); }

////////////////////////////////////////////////////////////////////////////////////////////////////

inline MkDuration Milliseconds(const mkint64 unitCnt)
{ return MkDuration(unitCnt, MkDuration::eMilliseconds); }

////////////////////////////////////////////////////////////////////////////////////////////////////

inline MkDuration Seconds(const mkint64 unitCnt)
{ return MkDuration(unitCnt, MkDuration::eSeconds); }

////////////////////////////////////////////////////////////////////////////////////////////////////

inline MkDuration Minutes(const mkint64 unitCnt)
{ return MkDuration(unitCnt, MkDuration::eMinutes); }
