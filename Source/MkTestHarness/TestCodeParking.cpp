////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
// Wx includes.
#include "InputOutput\MkDebugText.h"
#include "Process\MkCallStack.h"

// C++ includes.
#include <format>


// Test code (surprise!).
int Test()
{
    std::vector<mkuint64> addresses;
    std::vector<MkCallStack::StackInfo> stackInfos;

    void* pAddresses = &addresses;

    MkCallStack::Capture(addresses, 0, 16);
    MkCallStack::GetStackInformation(addresses, false, stackInfos);

    for (auto& infoIter : stackInfos)
    {
        MkAString formattedStr(std::format("{}({})", infoIter.GetFile().GetCString(), infoIter.GetLine()));
        MkDebugText() << formattedStr << ".\n";
    }

    MkConstAString patternStr = MkConstAString("%??*[abc][x-z]{123|456|789}$");
    MkStringPattern pattern = MkStringPattern(patternStr);

    MkAString candidateStr = MkAString("4abcy456");

    bool matchResult = pattern.Match(candidateStr);

    return matchResult ? 1 : 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
#include "Basic\MkMemoryVessel.h"
#include "TestBasic\MkTestPolymorphic.h"


// Test code (surprise!).
int Test()
{
    auto rTest = std::make_shared<MkTestPolymorphic2>();
    rTest->SetValue(27);

    MkMemoryVessel memoryVessel(MkMemoryVessel::eSmall);

    memoryVessel.WriteSharedPtr(rTest);

    memoryVessel.ResetForRead();
    {
        auto rTest2 = memoryVessel.ReadSharedPtr<MkTestPolymorphic>();
    }

    return 1;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
#include "Process\MkThread.h"

class MkTestThread : public MkThread
{
public:
    MkTestThread() :
        MkThread("TestThread")
    { StartThread(); }

    ~MkTestThread() override = default;

    void Execute() override
    {
        mkuint64 counter = 0;
        while (true)
        {
            ++counter;
        }
    }
};


// Test code (surprise!).
int Test()
{
    {
        auto testThread = MkTestThread();

        mkuint64 counter = 0;
        for (mkuint idx = 0; idx < 10000000; ++idx)
        {
            counter += 4;
        }
    }

    return 1;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
// Wx includes.
#include "Basic\MkTime.h"
#include "InputOutput\MkDebugText.h"
#include "Process\MkOSEvent.h"
#include "Process\MkThread.h"
#include "Services\MkServiceMaster.h"

class TestThread : public MkThread
{
public:
    TestThread(mkuint index, const std::shared_ptr<MkOSEvent>& rEvent) :
        MkThread("Test"),
        m_index(index),
        m_rEvent(rEvent)
    { StartThread(); }

    void Execute() override
    {
        MkMultiOSEvent multiOSEvent;

        multiOSEvent.AddOSEvent(m_rExitEvent);      // Event 0.
        multiOSEvent.AddOSEvent(m_rEvent);          // Event 1.

        while (true)
        {
            auto waitStatus = multiOSEvent.WaitForObjects(MkForever());

            switch (waitStatus)
            {
            case MkWaitStatus::eError :
            case MkWaitStatus::eTimeOut :
                break;

            default :
            {
                switch (waitStatus - MkWaitStatus::eEvent0)
                {
                case 0 :
                    // Exit the thread.
                    return;

                case 1:
                    MkDebugText() << "Task " << m_index << " fired.\n";
                    continue;

                default :
                    // An undefined event.
                    throw MkException(eOSFailure, "Undefined event.");
                }

                break;
            }
            }
        }
    }

private:
    mkuint m_index;
    std::shared_ptr<MkOSEvent> m_rEvent;
};

#if 0
int Test()
{
    MkServiceMaster& mkServiceMaster = MkServiceMaster::Instance();

    MkCPUTimer timer;
    for (mkuint idx = 0; idx < 2560; ++idx)
    {
        MkCPUTime time;
    }
    MkDebugText() << "It took " << timer.GetAString(MkDuration::eMicroseconds).GetCString() << ".\n";

    auto rEvent = std::make_shared<MkOSEvent>("Event");

    auto rTask0 = std::make_shared<TestThread>(0, rEvent);
    auto rTask1 = std::make_shared<TestThread>(1, rEvent);
    auto rTask2 = std::make_shared<TestThread>(2, rEvent);

    Sleep(3000);

    rEvent->Signal();
    MkDebugText() << "Signalled the first time.\n";

    Sleep(1000);

    for (mkuint idx = 0; idx < 256; ++idx)
    {
        rEvent->Signal();
        Sleep(2);
    }
    MkDebugText() << "Signalled the second time.\n";

    return 1;
}
#endif
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
// Mk includes.
#include "MkFoundation.h"
#include "Basic\MkString.h"
#include "TestClient\MkSleep.h"
#include "TestClient\MkSleepClient.h"

// C++ includes.
#include <iostream>

int main()
{
    std::vector<std::pair<MkAString, MkAString>> serviceAddresses;
    //serviceAddresses.push_back(std::make_pair(MkConstAString("127.0.0.1"), MkConstAString("12151")));
    serviceAddresses.push_back(std::make_pair(MkConstAString("127.0.0.1"), MkConstAString("255")));

    //serviceAddresses.push_back(std::make_pair(MkConstAString("10.5.0.2"), MkConstAString("255")));

    // Google Atlanta
    //serviceAddresses.push_back(std::make_pair(MkConstAString("74.125.138.101"), MkConstAString("80")));
    // Google Miami
    //serviceAddresses.push_back(std::make_pair(MkConstAString("172.217.165.206"), MkConstAString("80")));

    std::vector<std::shared_ptr<MkResourceLabel>> resourceLabels;

    auto rSleepClient = std::make_shared<MkSleepClient>(
            MkConstAString("Sleep"),
            serviceAddresses,
            resourceLabels,
            false);
    rSleepClient->Sleep(MkDuration(3ll, MkDuration::eMinutes));

    mkuint test = 1;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
// C++ includes.
#include <iostream>

// Windows includes.
#include <windows.h>

// Test code (surprise!).
int Test()
{
    SetConsoleOutputCP(CP_UTF8);

#if 1
    std::string str = "\xe2\x96\x91 ABC"; 
    std::cout << reinterpret_cast<const char*>(str.c_str()) << std::endl;
#else
    std::u8string u8Str = u8"C++20 Unicode 🚀";
    //std::u8string u8Str = u8"C++20 Unicode \xf0\x9f\x9a\x80";
    std::cout << reinterpret_cast<const char*>(u8Str.c_str()) << std::endl;
#endif

    return 1;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
// Mk includes.
#include "Basic\MkStringPattern.h"

// Test code (surprise!).
int Test()
{
    //MkAString testStr("'a+'$");
    //MkAString testStr("'{'a?*x*'|acccxx}'x$");
    MkAString testStr("{'a?*xx*'|accx}x$");
    //MkAString testStr("'x{'ab+'|'a?+'}'$");
    MkStringPattern testPattern(testStr);
    if (!testPattern.IsValid())
    {
        std::cout << "INVALID PATTERN\n";
        return 0;
    }

    if (!testPattern.Match(MkConstAString("accxx")))
    //if (!testPattern.Match(MkConstAString("aaa")))
    {
        std::cout << "DID NOT MATCH\n";
        return 0;
    }

#if 0
    std::cout << "Markers at " << testPattern.GetMarker(0)->GetCharacterIndex() <<
            " and " << testPattern.GetMarker(1)->GetCharacterIndex() << ".\n";
#endif

    std::cout << "Alternate " << testPattern.GetAlternates(0)->GetAlternateIndex() << " matched.\n";

#if 1
    std::cout << "Markers at " << testPattern.GetMatchedAlternate(0).GetMarker(0)->GetCharacterIndex() <<
            " and " << testPattern.GetMatchedAlternate(0).GetMarker(1)->GetCharacterIndex() << ".\n";
#endif

    return 1;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
// Winsock client example code.
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int __cdecl main(int argc, char **argv) 
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    const char *sendbuf = "this is a test";
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;
    
    // Validate the parameters
    if (argc != 2) {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %ld\n", iResult);

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    do {

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if ( iResult > 0 )
            printf("Bytes received: %d\n", iResult);
        else if ( iResult == 0 )
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while( iResult > 0 );

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
// Winsock server example code.
#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int __cdecl main(void) 
{
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    closesocket(ListenSocket);

    // Receive until the peer shuts down the connection
    do {

        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);

        // Echo the buffer back to the sender
            iSendResult = send( ClientSocket, recvbuf, iResult, 0 );
            if (iSendResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }
            printf("Bytes sent: %d\n", iSendResult);
        }
        else if (iResult == 0)
            printf("Connection closing...\n");
        else  {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

    } while (iResult > 0);

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}
#endif
