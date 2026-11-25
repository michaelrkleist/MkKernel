////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFramework.h"
#include "Basic\MkString.h"
#include "Process\MkScheduler.h"
#include "Process\MkSingleton.h"
#include "Services\MkService.h"

// Forward declarations.
class MkService;
class MkSocketAcceptor;
class MkSocketServer;

// A service master is a singleton for managing the services available from the associated
// process.
// o The service master listens for requests for a service using a MBTP socket.
// o A service request includes a service name and a list of resource labels for
//   required service resources.
// o If the service master has the requested service, it returns a positive MBTP
//   response including the count of pending service requests and hands the MBTP
//   socket to the service.
// o If the service master does not have requested service, it returns a negative
//   MBTP response and closes the MBTP socket.


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkServiceMaster
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkServiceMaster : public MkSingleton<MkServiceMaster>
{
public:
    MK_EXPORT_FRAMEWORK MkServiceMaster();
    MK_EXPORT_FRAMEWORK ~MkServiceMaster() override = default;

    bool MK_EXPORT_FRAMEWORK Execute(const MkAString& portStr);
    void MK_EXPORT_FRAMEWORK AddService(const MkAString& name, const std::shared_ptr<MkService>& rService);

private:
    friend class ConnectServiceTask;
    std::shared_ptr<MkService> MK_EXPORT_FRAMEWORK GetService(const MkAString& name);

    friend class AcceptTask;

    std::shared_ptr<MkSocketAcceptor> m_rRequestAcceptor;
    std::map<MkAString, std::shared_ptr<MkService>> m_services;
    MkCriticalSectionLock m_servicesLock;
};

