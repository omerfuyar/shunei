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
    SHUResult_Pending,
    SHUResult_ErrBadStructData,
    SHUResult_ErrNullPointer,
    SHUResult_ErrNetwork,
} SHUResult;

/// @brief Struct to hold necessary information for a client connection.
/// @note !!! USE ONLY WITH FUNCTIONS MADE FOR THIS STRUCT, NEVER WRITE MANUALLY !!!
typedef struct SHUConnection
{
    struct sockaddr_in address;

#ifdef _WIN32
    SOCKET fileDescriptor;
    int addressLength;
#else
    int fileDescriptor;
    socklen_t addressLength;
#endif
} SHUConnection;

/// @brief Struct to hold necessary information for a listener connection.
/// @note !!! USE ONLY WITH FUNCTIONS MADE FOR THIS STRUCT, NEVER WRITE MANUALLY !!!
typedef struct SHUListener
{
    struct sockaddr_in address;

#ifdef _WIN32
    SOCKET fileDescriptor;
    int addressLength;
#else
    int fileDescriptor;
    socklen_t addressLength;
#endif

    SHUConnection *clientConnections;
    unsigned long long clientCount;
} SHUListener;

/// @brief Initializes the network subsystem. Must be called before any other shunei function. See SHU_TerminateNetwork for cleanup.
/// @return Result of the operation. See SHUResult for details.
SHUResult SHU_InitializeNetwork(void);

/// @brief Terminates the network subsystem and releases any associated resources. Must be called when network operations are no longer needed. Automatically calls on exit if network is initialized. See SHU_InitializeNetwork for initialization.
/// @return Result of the operation. See SHUResult for details.
SHUResult SHU_TerminateNetwork(void);

/// @brief Creates and configures a client connection according to the specified type and parameters.
/// @param retConnection Pointer to a SHUConnection struct where the created connection information will be stored. Must not be NULL.
/// @param ip IP address to connect to, depending on the connection type.
/// @param port Port number to connect to.
/// @return Result of the operation. See SHUResult for details.
/// @note For server types, this function creates a listening socket bound to the specified IP and port.
SHUResult SHU_ConnectionCreateClient(SHUConnection *retConnection, const char *ip, unsigned short port);

/// @brief Creates and configures a listener connection according to the specified type and parameters.
/// @param retListener Pointer to a SHUListener struct where the created listener information will be stored. Must not be NULL.
/// @param ip IP address to bind to, depending on the connection type. Can be NULL to bind to all interfaces.
/// @param port Port number to bind to.
/// @param clientConnectionsBuffer Buffer to store client connections.
/// @param maxClientConnections Maximum number of client connections to handle.
/// @return Result of the operation. See SHUResult for details.
SHUResult SHU_ConnectionCreateListener(SHUListener *retListener, const char *ip, unsigned short port, SHUConnection *clientConnectionsBuffer, unsigned long long maxClientConnections);

/// @brief !!! DO NOT CALL THIS FUNCTION MANUALLY, USE SHU_ConnectionDestroy INSTEAD !!!
SHUResult SHUI_ConnectionDestroy(SHUConnection *connection);

/// @brief Destroys a connection and releases any associated resources. Can be used for both client and listener connections.
/// @param connection Pointer to the SHUConnection/SHUListener struct representing the connection to be destroyed. Must not be NULL.
/// @return Result of the operation. See SHUResult for details.
#define SHU_ConnectionDestroy(connection) SHUI_ConnectionDestroy((SHUConnection *)connection)

/// @brief Checks for a waiting client in a listener.
/// @param listener Listener to check for waiting clients.
/// @return Result of the operation. Pending if there is no connecting clients, Ok if there is at least one client waiting to be accepted. See SHUResult for details.
SHUResult SHU_ConnectionCheckListener(const SHUListener *listener, SHUConnection *retClientConnection);

/// @brief Sends data through a connection.
/// @param connection Pointer to the SHUConnection struct representing the connection to send data through. Must not be NULL.
/// @param data Pointer to the data to send.
/// @param dataSize Size of the data to send.
/// @return Result of the operation. See SHUResult for details.
SHUResult SHU_ConnectionSend(const SHUConnection *connection, const char *data, int dataSize);

/// @brief Receives data through a connection.
/// @param connection Pointer to the SHUConnection struct representing the connection to receive data from. Must not be NULL.
/// @param buffer Pointer to the buffer where received data will be stored.
/// @param bufferSize Size of the buffer.
/// @return Result of the operation. Pending if there is no data to receive, Ok if data was received successfully. See SHUResult for details.
SHUResult SHU_ConnectionReceive(const SHUConnection *connection, char *buffer, int bufferSize);

#pragma endregion Shunei Declarations

#pragma region Shunei Definitions

#ifdef SHUNEI_IMPLEMENTATION

#include <stddef.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#define SHUI_CheckConnection(connection) ((connection)->fileDescriptor != INVALID_SOCKET && \
                                          (connection)->addressLength > 0 &&                \
                                          (connection)->address.sin_family == AF_INET &&    \
                                          (connection)->address.sin_port != 0)
#else
#include <fcntl.h>
#include <arpa/inet.h>
#define SHUI_CheckConnection(connection) ((connection)->fileDescriptor >= 0 &&           \
                                          (connection)->addressLength > 0 &&             \
                                          (connection)->address.sin_family == AF_INET && \
                                          (connection)->address.sin_port != 0)
#endif

static SHUResult (*SHUI_AT_EXIT_FUNCTION)(void) = NULL;

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

SHUResult SHU_ConnectionCreateClient(SHUConnection *retConnection, const char *ip, unsigned short port)
{
    if (retConnection == NULL || port == 0 || ip == NULL)
    {
        return SHUResult_ErrNullPointer;
    }

    retConnection->fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);

#ifdef _WIN32
    if (retConnection->fileDescriptor == INVALID_SOCKET)
    {
        return SHUResult_ErrNetwork;
    }
#else
    if (retConnection->fileDescriptor < 0)
    {
        return SHUResult_ErrNetwork;
    }
#endif

    memset(&retConnection->address, 0, sizeof(retConnection->address));
    retConnection->address.sin_family = AF_INET;
    retConnection->address.sin_port = htons(port);
    retConnection->addressLength = sizeof(struct sockaddr_in);

    if (ip == NULL)
    {
        retConnection->address.sin_addr.s_addr = INADDR_ANY;
    }
    else if (inet_pton(AF_INET, ip, &retConnection->address.sin_addr.s_addr) != 1)
    {
        return SHUResult_ErrNetwork;
    }

#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(retConnection->fileDescriptor, (long)FIONBIO, &mode);
#else
    int flags = fcntl(retConnection->fileDescriptor, F_GETFL, 0);
    fcntl(retConnection->fileDescriptor, F_SETFL, flags | O_NONBLOCK);
#endif

    connect(retConnection->fileDescriptor, (struct sockaddr *)&retConnection->address, sizeof(struct sockaddr_in));

    return SHUResult_Ok;
}

SHUResult SHU_ConnectionCreateListener(SHUListener *retListener, const char *ip, unsigned short port, SHUConnection *clientConnectionsBuffer, unsigned long long maxClientConnections)
{
    if (retListener == NULL || port == 0)
    {
        return SHUResult_ErrNullPointer;
    }

    retListener->clientConnections = clientConnectionsBuffer;
    retListener->clientCount = maxClientConnections;

    retListener->fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);

#ifdef _WIN32
    if (retListener->fileDescriptor == INVALID_SOCKET)
    {
        return SHUResult_ErrNetwork;
    }
#else
    if (retListener->fileDescriptor < 0)
    {
        return SHUResult_ErrNetwork;
    }
#endif

    memset(&retListener->address, 0, sizeof(retListener->address));
    retListener->address.sin_family = AF_INET;
    retListener->address.sin_port = htons(port);

    if (ip == NULL)
    {
        retListener->address.sin_addr.s_addr = INADDR_ANY;
    }
    else if (inet_pton(AF_INET, ip, &retListener->address.sin_addr.s_addr) != 1)
    {
        return SHUResult_ErrNetwork;
    }

#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(retListener->fileDescriptor, (long)FIONBIO, &mode);
#else
    int flags = fcntl(retListener->fileDescriptor, F_GETFL, 0);
    fcntl(retListener->fileDescriptor, F_SETFL, flags | O_NONBLOCK);
#endif

    int opt = 1;
    setsockopt(retListener->fileDescriptor, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    if (bind(retListener->fileDescriptor, (struct sockaddr *)&retListener->address, sizeof(struct sockaddr_in)) < 0)
    {
        return SHUResult_ErrNetwork;
    }

    if (listen(retListener->fileDescriptor, SHUM_LISTEN_CONNECTION_QUEUE) < 0)
    {
        return SHUResult_ErrNetwork;
    }

    retListener->addressLength = sizeof(struct sockaddr_in);

    return SHUResult_Ok;
}

SHUResult SHUI_ConnectionDestroy(SHUConnection *connection)
{
    if (connection == NULL)
    {
        return SHUResult_ErrNullPointer;
    }

    if (!SHUI_CheckConnection(connection))
    {
        return SHUResult_ErrBadStructData;
    }

#ifdef _WIN32
    closesocket(connection->fileDescriptor);
#else
    close(connection->fileDescriptor);
#endif

    memset(connection, 0x00, sizeof(SHUConnection));

    return SHUResult_Ok;
}

SHUResult SHU_ConnectionCheckListener(const SHUListener *listener, SHUConnection *retClientConnection)
{
    if (listener == NULL || retClientConnection == NULL)
    {
        return SHUResult_ErrNullPointer;
    }

    if (!SHUI_CheckConnection(listener))
    {
        return SHUResult_ErrBadStructData;
    }

    retClientConnection->addressLength = sizeof(struct sockaddr_in);
    retClientConnection->fileDescriptor = accept(listener->fileDescriptor, (struct sockaddr *)&retClientConnection->address, &retClientConnection->addressLength);

#ifdef _WIN32
    if (retClientConnection->fileDescriptor == INVALID_SOCKET)
    {
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            return SHUResult_Pending;
        }
        return SHUResult_ErrNetwork;
    }
#else
    if (retClientConnection->fileDescriptor < 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            return SHUResult_Pending;
        }
        return SHUResult_ErrNetwork;
    }
#endif

    return SHUResult_Ok;
}

SHUResult SHU_ConnectionSend(const SHUConnection *connection, const char *data, int dataSize)
{
    if (connection == NULL || data == NULL || dataSize <= 0)
    {
        return SHUResult_ErrNullPointer;
    }

    if (!SHUI_CheckConnection(connection))
    {
        return SHUResult_ErrBadStructData;
    }

    int bytesSent = send(connection->fileDescriptor, data, dataSize, 0);

    if (bytesSent < 0)
    {
#ifdef _WIN32
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            return SHUResult_Pending;
        }
#else
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            return SHUResult_Pending;
        }
#endif
        return SHUResult_ErrNetwork;
    }

    return SHUResult_Ok;
}

SHUResult SHU_ConnectionReceive(const SHUConnection *connection, char *buffer, int bufferSize)
{
    if (connection == NULL || buffer == NULL || bufferSize == 0)
    {
        return SHUResult_ErrNullPointer;
    }

    if (!SHUI_CheckConnection(connection))
    {
        return SHUResult_ErrBadStructData;
    }

    int bytesReceived = recv(connection->fileDescriptor, buffer, bufferSize, 0);

    if (bytesReceived < 0)
    {
#ifdef _WIN32
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            return SHUResult_Pending;
        }
#else
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            return SHUResult_Pending;
        }
#endif
        return SHUResult_ErrNetwork;
    }
    else if (bytesReceived == 0)
    {
        return SHUResult_ErrNetwork;
    }

    return SHUResult_Ok;
}

#endif

#pragma endregion Shunei Definitions
