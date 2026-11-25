////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFramework.h"
#include "Basic\MkException.h"
#include "Basic\MkMemoryVessel.h"
#include "Basic\MkString.h"
#include "Basic\MkUID.h"
#include "InputOutput\MkMBTP.h"
#include "Process\MkCriticalSection.h"
#include "Process\MkTask.h"

// STL includes.
#include <deque>

// Forward declarations.
class ClientData;
class MkMemoryVessel;
class MkResourceLabel;
class MkSocketClient;
class MkSocketServer;


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkService
////////////////////////////////////////////////////////////////////////////////////////////////////

// A service responds to client requests.  Clients are identified primarily by a
// response socket.  Clients send requests to the request socket and the service
// fulfills the requests. Sockets are held open until the client closes them.  Client
// requests are put in a pending requests queue by the HandleRequestTask task.  The
// service fulfills each request with a task (see MkTaskPool), rescheduling the task
// if appropriate.  If the request socket is closed by the client while the request is
// being fulfilled, fulfillment is completed but the fulfillment response is not sent.
// The design of a service determines how many requests can be fulfilled concurrently.
class MkService
{
public:
    MK_EXPORT_FRAMEWORK MkService(const MkAString& name, mkuint32 maxActiveRequests) :
        m_name(name),
        m_maxActiveRequests(maxActiveRequests),
        m_resourceLabels()
    { }
    virtual ~MkService() = default;

    const MkAString& GetName() const
    { return m_name; }

    bool MK_EXPORT_FRAMEWORK Satisfies(
            const std::vector<std::shared_ptr<MkResourceLabel>>& clientResourceLabels) const;

    mkuint32 MK_EXPORT_FRAMEWORK AddClient(std::shared_ptr<MkSocketServer>& rRequestSocket);

    mkuint32 GetPendingRequestCount() const
    {
        MkCriticalSection cs(m_pendingRequestsLock);
        return (mkuint32)m_pendingRequests.size();
    }

    // NOTE: These request codes are purposely set to higher values so that individual
    // services can use values starting a 0 to identify their services.  When this base
    // service class reads the request, it peels off these requests to handle itself
    // before passing the request code and UID to derived service classes.
    enum RequestCode : mkuint32
    {
        eServiceConnect = 1000,
        eServiceRequest,            // The request code comes next.
    };

    enum ConnectionStatus : mkuint32
    {
        eAvailable = 200,
        eNotAvailable,
        eResourceNotAvailable,
        eResponseSocketInvalid,
        eInvalidConnection
    };

    enum ResponseCode : mkuint32
    {
        eRequestComplete,
        eInvalidRequest,        // TODO: Might not be useful.  MRK.
        eRequestFailed,         // TODO: Might not be useful.  MRK.
        ePendingRequestCnt
    };

    struct Request
    {
        Request() :
            m_clientIdx(0),
            m_rRequestMessage(nullptr)
        { }

        Request(const mkuint32 clientIdx, const std::shared_ptr<MkMemoryVessel>& rRequestMessage) :
            m_clientIdx(clientIdx),
            m_rRequestMessage(rRequestMessage)
        { }

        bool IsNull() const
        { return bool(m_clientIdx == 0); }

        const mkuint32 m_clientIdx;
        const std::shared_ptr<MkMemoryVessel> m_rRequestMessage;
    };

    struct Response
    {
        Response(const Request& request, MkMemoryVessel::BlockCapacity responseBlockCapacity) :
            m_clientIdx(request.m_clientIdx),
            m_rResponseMessage(std::make_shared<MkMemoryVessel>(responseBlockCapacity))
        {
            m_rResponseMessage->WriteObject(MkMBTPHeader());

            MkUID requestUID(false);;
            request.m_rRequestMessage->ReadObject(requestUID);
            m_rResponseMessage->WriteEnum(eRequestComplete);
            m_rResponseMessage->WriteObject(requestUID);
        }

        mkuint32 m_clientIdx;
        std::shared_ptr<MkMemoryVessel> m_rResponseMessage;
    };

    virtual const Response ProcessRequest(const Request& request) = 0;

protected:
    void MK_EXPORT_FRAMEWORK AddResourceLabel(const std::shared_ptr<MkResourceLabel>& rResourceLabel)
    { m_resourceLabels.push_back(rResourceLabel); }

private:
    mkuint32 GetClientIndex()
    { return ++m_clientIdx; }

    friend class ConnectServiceTask;
    friend class ServiceSocketEventFunctor;
    friend class HandleRequestTask;

    void MK_EXPORT_FRAMEWORK RemoveClient(const mkuint32 clientIdx);

    class ServiceTask : public MkTask
    {
    public:
        ServiceTask(const MkAString& name, MkService* pService) :
            MkTask(name),
            m_pService(pService)
        { }

        Status Execute() override;

    private:
        // NOTE: The MkService reference here might need to be changed to a weak
        // reference for service lifetime reasons.  However, at this time, I cannot
        // see a reason for an executable to have service(s) that come and go.  MRK.
        MkService* m_pService;
    };

    Request GetPendingTask();
    void SendResponse(const Response& serviceResponse);
    bool SendMessage(MkSocketIO& responseSocket, MkMemoryVessel& responseMessage) const;
    
    MkAString m_name;
    mkuint32 m_maxActiveRequests;
    std::vector<std::shared_ptr<MkResourceLabel>> m_resourceLabels;

    class ClientData
    {
    public:
        ClientData() = default;
        ClientData(
                const std::shared_ptr<MkSocketServer>& rRequestSocket,
                const std::shared_ptr<MkMBTPReader>& rMBTPReader) :
            m_rRequestSocket(rRequestSocket),
            m_rMBTPReader(rMBTPReader)
        { }
        ~ClientData() = default;

    private:
        friend class MkService;
        friend class ServiceSocketEventFunctor;
        friend class HandleRequestTask;

        std::shared_ptr<MkSocketServer> m_rRequestSocket;
        std::shared_ptr<MkMBTPReader> m_rMBTPReader;
    };

    std::map<mkuint32, ClientData> m_clientMap;
    mutable MkCriticalSectionLock m_clientMapLock;

    // NOTE: For a bit of simplicity, one lock protects both the pending requests
    // and the active requests count.  MRK.
    std::deque<Request> m_pendingRequests;
    mkuint32 m_activeRequestCnt = 0;
    mutable MkCriticalSectionLock m_pendingRequestsLock;

    std::atomic<mkuint32> m_clientIdx = 100;
};