////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "TestLibrary\MkTestLibraryPCH.h"
#include "TestClient\MkSleepClient.h"

// Wx includes.
#include "Basic\MkMemoryVessel.h"
#include "TestClient\MkSleepService.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkSleepClient
////////////////////////////////////////////////////////////////////////////////////////////////////

MkSleepClient::MkSleepClient(
        const MkAString& serviceName,
        const std::vector<MkIP4PortString>& serviceAddresses,
        const std::vector<std::shared_ptr<MkResourceLabel>>& resourceLabels,
        const bool loadBalance) :
    MkClient(serviceName, serviceAddresses, resourceLabels, loadBalance)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::optional<MkUID> MkSleepClient::Sleep(
        const MkDuration duration, const std::shared_ptr<MkOSEvent>& rResponseEvent)
{
    if (GetActiveServiceCount() == 0) return std::optional<MkUID>();

    // Assemble the base request message.
    auto rRequestMessage = std::make_shared<MkMemoryVessel>(MkMemoryVessel::eTiny);
    MkUID requestUID;
    FillBaseRequest(rRequestMessage, requestUID, MkSleepService::eSleep);

    // Add the method specific arguments.
    rRequestMessage->WriteObject(duration);

    //if (!MakeRequest(rRequestMessage, requestUID, nullptr, MkDuration(5.0, MkDuration::eSeconds)))
    if (!MakeRequest(rRequestMessage, requestUID, nullptr, MkDuration()))
    {
        return std::optional<MkUID>();
    }
    
    return std::optional<MkUID>(requestUID);
}