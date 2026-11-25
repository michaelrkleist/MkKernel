////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#if 1
// Mk includes.
#include "MkFoundation.h"
#include "Basic\MkString.h"
#include "Process\MkFrameworkApplication.h"
#include "Services\MkServiceMaster.h"
#include "TestClient\MkSleepService.h"

// C++ includes.
#include <iostream>


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkServiceMain Main
////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    // Grab the port number, if any.
    MkAString portStr;
    if (argc > 1)
    {
        portStr = MkAString(argv[1]);
    }

    // Get the framework up and running.
    MkFrameworkApplication::Instance("ServiceMain").Start();

    MkServiceMaster& serviceMaster = MkServiceMaster::Instance();

    auto rSleepService = std::make_shared<MkSleepService>("Sleep");
    serviceMaster.AddService(rSleepService->GetName(), rSleepService);

    if (!serviceMaster.Execute(portStr))
    {
        std::cerr << "Cannot set up listening socket\n";
        return 0;
    }

    for (mkuint idx = 0; idx < 1'000'000; ++idx)
    {
        Sleep(1000);
    }

    MkFrameworkApplication::Instance().Stop();
}
#endif

