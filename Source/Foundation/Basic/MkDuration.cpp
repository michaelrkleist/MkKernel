////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Foundation\MkFoundationPCH.h"
#include "Basic\MkDuration.h"

// Static initializations.
namespace
{
MkConstAString sUnitStrs[MkDuration::eUnitCnt] = {
        MkConstAString("ns"),
        MkConstAString("100ns"),
        MkConstAString("us"),
        MkConstAString("ms"),
        MkConstAString("s"),
        MkConstAString("m"),
        MkConstAString("h"),
        MkConstAString("d"),
        MkConstAString("w") };

mkint64 sUnitPerDuration[MkDuration::eUnitCnt] = {
        100ll,          // Special case in the code.
        1ll,
        10ll,
        10'000ll,
        10'000'000ll,
        10'000'000ll * 60ll,
        10'000'000ll * 60ll * 60ll,
        10'000'000ll * 60ll * 60ll * 24ll,
        10'000'000ll * 60ll * 60ll * 24ll * 7ll };

mkint64 sForever = 9'000'000'000'000'000'000ll;
MkConstAString sForeverStr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkDuration
////////////////////////////////////////////////////////////////////////////////////////////////////

MkDuration::MkDuration(const mkint64 unitCnt, const MkDuration::TimeUnits units)
{
    m_duration = (units == MkDuration::eNanoseconds ?
            unitCnt / 100ll : unitCnt * sUnitPerDuration[units]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkDuration::MkDuration(const mkdouble unitScalar, const MkDuration::TimeUnits units)
{
    m_duration = (units == MkDuration::eNanoseconds ? 
            unitScalar * 0.01 : unitScalar * (mkfloat64)sUnitPerDuration[units]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

mkfloat64 MkDuration::GetScalar(const TimeUnits units) const
{
    return (units == MkDuration::eNanoseconds ? 
            mkfloat64(m_duration) * 0.01 : mkfloat64(m_duration) / (mkfloat64)sUnitPerDuration[units]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

mkint64 MkDuration::GetIntegral(const TimeUnits units) const
{
    return (units == MkDuration::eNanoseconds ? m_duration / 100 : m_duration / sUnitPerDuration[units]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkDuration::SetToForever()
{
    m_duration = sForever;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkDuration::IsForever() const
{
    return m_duration == sForever;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkAString MkDuration::GetAString(const TimeUnits units) const
{
    if (IsForever()) return sForeverStr;

    mkfloat64 unitScalar = (units == MkDuration::eNanoseconds ? 
            mkfloat64(m_duration) * 0.01 : mkfloat64(m_duration) / (mkfloat64)sUnitPerDuration[units]);

    MkAString durationStr("%010.6f%s", fmod(unitScalar, 1000.0), sUnitStrs[units].GetCString());
    return durationStr;
}
