#pragma once

#pragma region Shunei Macros

#define SHUM_ADDRESS_STRLEN 32
#define SHUM_LISTEN_CONNECTION_QUEUE 8

#pragma endregion Shunei Macros

#pragma region Shunei Declarations

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

/// @brief Result values that can be returned by shunei functions.
typedef enum SHUResult
{
    SHUResult_Ok = 0,
    SHUResult_Err,
} SHUResult;

/// @brief Struct to hold necessary information for a connection, used for all types of SHUConnectionType types.
/// @note !!! USE ONLY WITH FUNCTIONS MADE FOR THIS STRUCT, NEVER WRITE MANUALLY !!!
typedef struct SHUConnection
{
    unsigned long long clientCount;
    struct sockaddr_in address;
    void *clientSockets;

#ifdef _WIN32
    SOCKET fileDescriptor;
    int addressLength;
#else
    int fileDescriptor;
    socklen_t addressLength;
#endif
} SHUConnection;

/// @brief Initializes the network subsystem. Must be called before any other shunei function. See SHU_TerminateNetwork for cleanup.
/// @return Result of the operation. See SHUResult enum for possible values.
SHUResult SHU_InitializeNetwork(void);

/// @brief Terminates the network subsystem and releases any associated resources. Must be called when network operations are no longer needed. Automatically calls on exit if network is initialized. See SHU_InitializeNetwork for initialization.
/// @return Result of the operation. See SHUResult enum for possible values.
SHUResult SHU_TerminateNetwork(void);

/// @brief Creates and configures a connection according to the specified type and parameters.
/// @param retConnection Pointer to a SHUConnection struct where the created connection information will be stored. Must not be NULL.
/// @param clientCount Leave 0 to create a client connection, or specify the maximum number of clients for a server connection.
/// @param ip IP address to bind or connect to, depending on the connection type. For server, this can be NULL to bind to all interfaces. For client types, this must be a valid IP address string.
/// @param port Port number to bind or connect to.
/// @return Result of the operation. See SHUResult enum for possible values.
/// @note For server types, this function creates a listening socket bound to the specified IP and port.
SHUResult SHU_ConnectionCreate(SHUConnection *retConnection, unsigned long long clientCount, const char *ip, unsigned short port);

/// @brief Destroys a connection and releases any associated resources.
/// @param connection Pointer to the SHUConnection struct representing the connection to be destroyed. Must not be NULL.
/// @return Result of the operation. See SHUResult enum for possible values.
SHUResult SHU_ConnectionDestroy(SHUConnection *connection);

/// @brief Sends data through a connection.
/// @param connection Pointer to the SHUConnection struct representing the connection to send data through. Must not be NULL.
/// @param data Pointer to the data to send.
/// @param dataSize Size of the data to send.
/// @return Result of the operation. See SHUResult enum for possible values.
SHUResult SHU_ConnectionSend(const SHUConnection *connection, const void *data, unsigned long long dataSize);

/// @brief Receives data through a connection.
/// @param connection Pointer to the SHUConnection struct representing the connection to receive data from. Must not be NULL.
/// @param buffer Pointer to the buffer where received data will be stored.
/// @param bufferSize Size of the buffer.
/// @return Result of the operation. See SHUResult enum for possible values.
SHUResult SHU_ConnectionReceive(const SHUConnection *connection, void *buffer, unsigned long long bufferSize);

#pragma endregion Shunei Declarations

#pragma region Shunei Definitions

#ifdef SHUNEI_IMPLEMENTATION

#include <stddef.h>
#include <string.h>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

void (*SHUI_AT_EXIT_FUNCTION)(void) = NULL;

#pragma region Shunei Internals

static void SHUI_AT_EXIT(void)
{
    if (SHUI_AT_EXIT_FUNCTION != NULL)
    {
        SHUI_AT_EXIT_FUNCTION();
    }
}

#pragma endregion Shunei Internals

SHUResult SHU_InitializeNetwork(void)
{
    SHUI_AT_EXIT_FUNCTION = SHU_TerminateNetwork;
    atexit(SHUI_AT_EXIT);

#ifdef _WIN32
    WSADATA wsa;
    return (SHUResult)WSAStartup(MAKEWORD(2, 2), &wsa);
#else
    return SHUResult_Ok;
#endif
}

SHUResult SHU_TerminateNetwork(void)
{
    SHUI_AT_EXIT_FUNCTION = NULL;

#ifdef _WIN32
    return (SHUResult)WSACleanup();
#else
    return SHUResult_Ok;
#endif
}

SHUResult SHU_ConnectionCreate(SHUConnection *retConnection, unsigned long long clientCount, const char *ip, unsigned short port)
{
    retConnection->fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);

#ifdef _WIN32
    if (retConnection->fileDescriptor == INVALID_SOCKET)
    {
        return SHUResult_Err;
    }
#else
    if (retConnection->fileDescriptor < 0)
    {
        return SHUResult_Err;
    }
#endif

    memset(&retConnection->address, 0, sizeof(retConnection->address));
    retConnection->address.sin_family = AF_INET;
    retConnection->address.sin_port = htons(port);

    if (ip == NULL)
    {
        retConnection->address.sin_addr.s_addr = INADDR_ANY;
    }
    else if (inet_pton(AF_INET, ip, &retConnection->address.sin_addr.s_addr) != 1)
    {
        return SHUResult_Err;
    }

    if (clientCount)
    {
        int opt = 1;
        setsockopt(retConnection->fileDescriptor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        bind(retConnection->fileDescriptor, (struct sockaddr *)&retConnection->address, sizeof(struct sockaddr_in));

        listen(retConnection->fileDescriptor, SHUM_LISTEN_CONNECTION_QUEUE);
    }
    else
    {
    }

    return SHUResult_Ok;
}

SHUResult SHU_ConnectionDestroy(SHUConnection *connection)
{
    (void)connection;
    return SHUResult_Ok;
}

SHUResult SHU_ConnectionSend(const SHUConnection *connection, const void *data, unsigned long long dataSize)
{
    (void)connection;
    (void)data;
    (void)dataSize;
    return SHUResult_Ok;
}

SHUResult SHU_ConnectionReceive(const SHUConnection *connection, void *buffer, unsigned long long bufferSize)
{
    (void)connection;
    (void)buffer;
    (void)bufferSize;
    return SHUResult_Ok;
}

#endif

#pragma endregion Shunei Definitions
