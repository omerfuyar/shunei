#pragma once

#pragma region Shunei Macros

#define SHUM_ADDRESS_STRLEN 32

#pragma endregion Shunei Macros

#pragma region Shunei Declarations

/// @brief Result values that can be returned by shunei functions.
typedef enum SHUResult
{
    SHUResult_Ok = 0,
    SHUResult_Err,
} SHUResult;

/// @brief Type of connections supported by shunei.
typedef enum SHUConnectionType
{
    SHUConnectionType_Server,
    SHUConnectionType_Client,
    SHUConnectionType_PeerHost,
    SHUConnectionType_PeerJoin,
} SHUConnectionType;

/// @brief
/// @note Use only with functions made for this struct, never write manually.
typedef struct SHUSocket
{
    struct sockaddr_in address;
#ifdef _WIN32
    SOCKET fileDescriptor;
    int addressLength;
#else
    int fileDescriptor;
    socklen_t addressLength;
#endif
} SHUSocket;

/// @brief Struct to hold necessary information for a connection, used for all types of SHUConnectionType types.
/// @note Use only with functions made for this struct, never write manually.
typedef struct SHUConnection
{
    SHUConnectionType type;
    SHUSocket socket;
    union
    {
        SHUSocket listener; // only used by server / p2p host
        struct
        {
            char ip[SHUM_ADDRESS_STRLEN];
            unsigned short port;
        } speaker; // only used by client / p2p join
    };
} SHUConnection;

SHUResult SHU_Initialize(void);

SHUResult SHU_Terminate(void);

SHUResult SHU_ConnectionCreate(SHUConnection *retConnection, SHUConnectionType type, const char *ip, unsigned short port);

SHUResult SHU_ConnectionDestroy(SHUConnection *connection);

SHUResult SHU_ConnectionSend(const SHUConnection *connection, const void *data, unsigned long long dataSize);

SHUResult SHU_ConnectionReceive(const SHUConnection *connection, void *buffer, unsigned long long bufferSize);

#pragma endregion Shunei Declarations

#pragma region Shunei Definitions

#ifdef SHUNEI_IMPLEMENTATION

#ifdef _WIN32
#include "winsock2.h"
#include "ws2tcpip.h"
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

SHUResult SHU_Initialize(void)
{
#ifdef _WIN32
    WSADATA wsa;
    return (SHUResult)WSAStartup(MAKEWORD(2, 2), &wsa);
#else
    return SHUResult_Ok;
#endif
}

SHUResult SHU_Terminate(void)
{
#ifdef _WIN32
    return (SHUResult)WSACleanup();
#else
    return SHUResult_Ok;
#endif
}

#endif

#pragma endregion Shunei Definitions
