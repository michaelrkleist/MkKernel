////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
// Mk includes.
#include "MkFoundation.h"
#include "Basic\MkString.h"
#include "Basic\MkStringPattern.h"

// C++ includes.
#include <iostream>

int Test();

int main()
{
    return Test();
}
#endif


#if 1
// Mk includes.
#include "MkFoundation.h"
#include "Basic\MkString.h"
#include "InputOutput\MkDebugText.h"
#include "Process\MkFrameworkApplication.h"
#include "TestClient\MkSleep.h"
#include "TestClient\MkSleepClient.h"

// C++ includes.
#include <iostream>

int main(int argc, char* argv[])
{
    std::vector<MkIP4PortString> serviceAddresses;
    for  (mkuint32 addressIdx = 1; addressIdx < argc; ++addressIdx)
    {
        serviceAddresses.push_back(MkAString(argv[addressIdx]));
    }

    // Get the framework up and running.
    MkFrameworkApplication::Instance("ClientTest").Start();

    std::vector<std::shared_ptr<MkResourceLabel>> resourceLabels;

    auto rSleepClient = std::make_shared<MkSleepClient>(
            MkAString("Sleep"),
            serviceAddresses,
            resourceLabels,
            false);

    auto optUID = rSleepClient->Sleep(MkDuration(10.0, MkDuration::eSeconds), nullptr);

    if (optUID.has_value())
    {
        { MkDebugSystemText() << "*** A sleep got sent. ***.\n"; }

        while (!rSleepClient->CheckResponse(optUID.value()))
        {
            Sleep(10);
        }

        auto rResponseMessage = rSleepClient->TakeResponse(optUID.value());

        {
            MkDebugSystemText() << "*** Hurray, a sleep sent a response of " <<
                    rResponseMessage->GetByteCount() << " bytes ***.\n";
        }
    }
    else
    {
        { MkDebugSystemText() << "??? A sleep did not get sent. ???.\n"; }
    }

    rSleepClient.reset();

    MkFrameworkApplication::Instance().Stop();
}
#endif


#if 0
// Mk includes.
#include "MkFoundation.h"
#include "Basic\MkString.h"
#include "InputOutput\MkDebugText.h"
#include "Process\MkFrameworkApplication.h"

// C++ includes.
#include <iostream>


static mkuint sSleepCnt = 0;

class Sleeper : public MkTask
{
public:
    Sleeper() :
        MkTask(MkConstAString("Sleeper"))
    { }

    MkTask::Status Execute() override
    {
        { MkDebugSystemText() << "Sleeping " << ++sSleepCnt << " times.\n";}

        DWORD dwMilliseconds = 200;
        Sleep(dwMilliseconds);
        return eComplete;
    }
};


int main()
{
    // Get the framework up and running.
    MkFrameworkApplication::Instance(MkConstAString("TaskPoolTest")).Start();

    for (mkuint loopIdx = 0; loopIdx < 1000; ++loopIdx)
    {
        Framework::TaskPool().Schedule(std::make_shared<Sleeper>(), MkTask::eNormal);

        DWORD dwMilliseconds = 300;
        Sleep(dwMilliseconds);
    }

     DWORD dwMilliseconds = 200000;
     Sleep(dwMilliseconds);

    MkFrameworkApplication::Instance().Stop();
}
#endif
