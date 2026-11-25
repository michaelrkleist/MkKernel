////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkTestLibrary.h"
#include "Services\MkClient.h"
#include "TestClient\MkSleep.h"

// STL includes.
#include <future>


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkSleepClient
////////////////////////////////////////////////////////////////////////////////////////////////////

class MK_EXPORT_TESTLIBRARY MkSleepClient : public MkSleep, public MkClient
{
public:
    MkSleepClient(
            const MkAString& serviceName,
            const std::vector<MkIP4PortString>& serviceAddresses,
            const std::vector<std::shared_ptr<MkResourceLabel>>& resourceLabels,
            const bool loadBalance);
    ~MkSleepClient() override = default;

    std::optional<MkUID> Sleep(
            const MkDuration duration, const std::shared_ptr<MkOSEvent>& rResponseEvent);
};