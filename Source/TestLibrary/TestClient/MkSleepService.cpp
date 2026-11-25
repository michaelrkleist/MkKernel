////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "TestLibrary\MkTestLibraryPCH.h"
#include "TestClient\MkSleepService.h"

// Wx includes.
#include "Basic\MkMemoryVessel.h"
#include "Services\MkServiceMaster.h"

#define DEBUG 0
#if DEBUG
#include "InputOutput\MkDebugText.h"
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkSleepService
////////////////////////////////////////////////////////////////////////////////////////////////////

const MkService::Response MkSleepService::ProcessRequest(const MkService::Request& request)
{
#if DEBUG
    { MkDebugSystemText() << "*** Entering sleep service process request.\n"; }
#endif

    MkService::Response response(request, MkMemoryVessel::eTiny);

    CommandCode commandCode;
    request.m_rRequestMessage->ReadEnum(commandCode);
    switch (commandCode)
    {
    case eSleep :
    {
        // Deserialize the arguments.
        MkDuration duration;
        request.m_rRequestMessage->ReadObject(duration);

        // Satisfy the request.
        MkSleep::Sleep(duration);

#if DEBUG
        { MkDebugSystemText() << "Sleep service process request completed.\n"; }
#endif
        break;
    }

    default :
        // An undefined command.
        throw MkException(eLogicError, "Invalid sleep service command.");
    }

    return response;
}