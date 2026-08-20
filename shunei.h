#pragma once

// todo proper error checking for socket operations

#ifndef SHU_HEADER
#ifdef SHU
#include SHU
#else
#include "../shu/shu.h"
#endif
#endif

#pragma region Macros

#define SHUM_ADDRESS_STRLEN 32
#define SHUM_LISTEN_CONNECTION_QUEUE 8

#pragma endregion Macros

#pragma region Declarations

// todo maybe move these to implementation
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

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
    SHUConnection connection;

    SHUConnection *clientConnections;
    usz clientCount;
    usz clientCapacity;
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
SHUResult SHU_ConnectionCreate(SHUConnection *retConnection, const char *ip, u16 port);

/// @brief Creates and configures a listener connection according to the specified type and parameters.
/// @param retListener Pointer to a SHUListener struct where the created listener information will be stored. Must not be NULL.
/// @param ip IP address to bind to, depending on the connection type. Can be NULL to bind to all interfaces.
/// @param port Port number to bind to.
/// @param clientConnectionsBuffer Buffer to store client connections.
/// @param maxClientConnections Maximum number of client connections to handle.
/// @return Result of the operation. See SHUResult for details.
SHUResult SHU_ListenerCreate(SHUListener *retListener, const char *ip, u16 port, SHUConnection *clientConnectionsBuffer, usz maxClientConnections);

/// @brief Destroys a connection and releases any associated resources.
/// @param connection Pointer to the SHUConnection to be destroyed.
/// @return Result of the operation. See SHUResult for details.
SHUResult SHU_ConnectionDestroy(SHUConnection *connection);

/// @brief Destroys a listener connection and releases any associated resources.
/// @param connection Pointer to the SHUListener to be destroyed.
/// @return Result of the operation. See SHUResult for details.
SHUResult SHU_ListenerDestroy(SHUListener *listener);

/// @brief Checks for a waiting client in a listener.
/// @param listener Listener to check for waiting clients.
/// @param retClientConnection pointer to the client connection that is connected to this listener if so.
/// @return Result of the operation. Pending if there is no connecting clients, Ok if there is at least one client waiting to be accepted. See SHUResult for details.
SHUResult SHU_ListenerCheck(SHUListener *listener, SHUConnection **retClientConnection);

/// @brief Waits for a client to connect.
/// @param listener Listener to wait for clients.
/// @param retClientConnection pointer to the client connection that is connected to this listener if so.
/// @return Result of the operation. Pending if there is no connecting clients, Ok if there is at least one client waiting to be accepted. See SHUResult for details.
SHUResult SHU_ListenerWait(SHUListener *listener, SHUConnection **retClientConnection);

/// @brief Sends data through a connection.
/// @param connection Pointer to the SHUConnection struct representing the connection to send data through. Must not be NULL.
/// @param data Pointer to the data to send.
/// @param dataSize Size of the data to send.
/// @param retSentSize Bytes sent in this batch. Leave NULL if not needed.
/// @return Result of the operation. See SHUResult for details.
SHUResult SHU_ConnectionSend(SHUConnection *connection, const char *data, usz dataSize, usz *retSentSize);

/// @brief Sends data through a connection, blocking until all of it has been sent.
/// @param connection Pointer to the SHUConnection struct representing the connection to send data through. Must not be NULL.
/// @param data Pointer to the data to send.
/// @param dataSize Size of the data to send.
/// @return Result of the operation. See SHUResult for details.
SHUResult SHU_ConnectionSendWait(SHUConnection *connection, const char *data, usz dataSize);

/// @brief Receives data through a connection.
/// @param connection Pointer to the SHUConnection struct representing the connection to receive data from. Must not be NULL.
/// @param buffer Pointer to the buffer where received data will be stored.
/// @param bufferSize Size of the buffer.
/// @param retReceivedSize Bytes received in this batch. Leave NULL if not needed.
/// @return Result of the operation. Pending if there is no data to receive, Ok if data was received successfully. See SHUResult for details.
SHUResult SHU_ConnectionReceive(SHUConnection *connection, char *buffer, usz bufferSize, usz *retReceivedSize);

/// @brief Receives data through a connection, blocking until at least some data has been received.
/// @param connection Pointer to the SHUConnection struct representing the connection to receive data from. Must not be NULL.
/// @param buffer Pointer to the buffer where received data will be stored.
/// @param bufferSize Size of the buffer.
/// @return Result of the operation. See SHUResult for details.
SHUResult SHU_ConnectionReceiveWait(SHUConnection *connection, char *buffer, usz bufferSize);

#pragma endregion Declarations

#pragma region Definitions

#ifdef SHU_IMPLEMENTATION

#include <string.h>
#include <errno.h>

#ifdef _WIN32
#define SHUI_CheckSocket(socket) (socket != INVALID_SOCKET)
#else
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/select.h>
#define SHUI_CheckSocket(socket) (socket >= 0)
#endif

#define SHUI_CheckPanicConnection(connection) SHU_Assert(SHUI_CheckSocket((connection)->fileDescriptor) &&  \
                                                             (connection)->addressLength > 0 &&             \
                                                             (connection)->address.sin_family == AF_INET && \
                                                             (connection)->address.sin_port != 0,           \
                                                         "Connection " #connection " is invalid.")

#pragma region Internals

static struct
{
    SHUResult (*atExitFunction)(void);
} SHUNEI = {0};

static void SHUNEI_AT_EXIT(void)
{
    if (SHUNEI.atExitFunction != NULL)
    {
        SHUNEI.atExitFunction();
    }
}

#pragma endregion Internals

SHUResult SHU_InitializeNetwork(void)
{
    SHUNEI.atExitFunction = SHU_TerminateNetwork;
    atexit(SHUNEI_AT_EXIT);

#ifdef _WIN32
    WSADATA wsa;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (result)
    {
        return SHUResult_ErrNetwork;
    }
#endif

    return SHUResult_Ok;
}

SHUResult SHU_TerminateNetwork(void)
{
    SHUNEI.atExitFunction = NULL;

#ifdef _WIN32
    int result = WSACleanup();
    if (result)
    {
        return SHUResult_ErrNetwork;
    }
#endif

    return SHUResult_Ok;
}

SHUResult SHU_ConnectionCreate(SHUConnection *retConnection, const char *ip, u16 port)
{
    SHU_AssertNullPointer(retConnection);
    SHU_AssertNullPointer(ip);

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

SHUResult SHU_ListenerCreate(SHUListener *retListener, const char *ip, u16 port, SHUConnection *clientConnectionsBuffer, usz maxClientConnections)
{
    SHU_AssertNullPointer(retListener);

    retListener->clientConnections = clientConnectionsBuffer;
    retListener->clientCapacity = maxClientConnections;
    retListener->clientCount = 0;

    retListener->connection.fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);

#ifdef _WIN32
    if (retListener->connection.fileDescriptor == INVALID_SOCKET)
    {
        return SHUResult_ErrNetwork;
    }
#else
    if (retListener->connection.fileDescriptor < 0)
    {
        return SHUResult_ErrNetwork;
    }
#endif

    memset(&retListener->connection.address, 0, sizeof(retListener->connection.address));
    retListener->connection.address.sin_family = AF_INET;
    retListener->connection.address.sin_port = htons(port);

    if (ip == NULL)
    {
        retListener->connection.address.sin_addr.s_addr = INADDR_ANY;
    }
    else if (inet_pton(AF_INET, ip, &retListener->connection.address.sin_addr.s_addr) != 1)
    {
        return SHUResult_ErrNetwork;
    }

#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(retListener->connection.fileDescriptor, (long)FIONBIO, &mode);
#else
    int flags = fcntl(retListener->connection.fileDescriptor, F_GETFL, 0);
    fcntl(retListener->connection.fileDescriptor, F_SETFL, flags | O_NONBLOCK);
#endif

    int opt = 1;
    setsockopt(retListener->connection.fileDescriptor, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    if (bind(retListener->connection.fileDescriptor, (struct sockaddr *)&retListener->connection.address, sizeof(struct sockaddr_in)) < 0)
    {
        return SHUResult_ErrNetwork;
    }

    if (listen(retListener->connection.fileDescriptor, SHUM_LISTEN_CONNECTION_QUEUE) < 0)
    {
        return SHUResult_ErrNetwork;
    }

    retListener->connection.addressLength = sizeof(struct sockaddr_in);

    return SHUResult_Ok;
}

SHUResult SHU_ConnectionDestroy(SHUConnection *connection)
{
    SHU_AssertNullPointer(connection);
    SHUI_CheckPanicConnection(connection);

#ifdef _WIN32
    if (closesocket(connection->fileDescriptor))
    {
        return SHUResult_ErrNetwork;
    }
#else
    if (close(connection->fileDescriptor))
    {
        return SHUResult_ErrNetwork;
    }
#endif

    memset(connection, 0x00, sizeof(SHUConnection));

    return SHUResult_Ok;
}

SHUResult SHU_ListenerDestroy(SHUListener *listener)
{
    SHU_CheckReturn(SHU_ConnectionDestroy(&listener->connection));

    listener->clientConnections = NULL;
    listener->clientCount = 0;
    listener->clientCapacity = 0;

    return SHUResult_Ok;
}

SHUResult SHU_ListenerCheck(SHUListener *listener, SHUConnection **retClientConnection)
{
    SHU_AssertNullPointer(listener);
    SHU_AssertNullPointer(retClientConnection);
    SHUI_CheckPanicConnection(&listener->connection);

    if (listener->clientCount >= listener->clientCapacity)
    {
        return SHUResult_ErrIndexOutOfBounds;
    }

    SHUConnection *connection = listener->clientConnections + listener->clientCount;

    connection->addressLength = sizeof(struct sockaddr_in);
    connection->fileDescriptor = accept(listener->connection.fileDescriptor, (struct sockaddr *)&connection->address, &connection->addressLength);

#ifdef _WIN32
    if (connection->fileDescriptor == INVALID_SOCKET)
    {
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            return SHUResult_Pending;
        }
        return SHUResult_ErrNetwork;
    }
#else
    if (connection->fileDescriptor < 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            return SHUResult_Pending;
        }
        return SHUResult_ErrNetwork;
    }
#endif

    *retClientConnection = connection;
    listener->clientCount++;

    return SHUResult_Ok;
}

SHUResult SHU_ListenerWait(SHUListener *listener, SHUConnection **retClientConnection)
{
    SHU_AssertNullPointer(listener);
    SHU_AssertNullPointer(retClientConnection);
    SHUI_CheckPanicConnection(&listener->connection);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(listener->connection.fileDescriptor, &fds);

    int selectResult = select((int)(listener->connection.fileDescriptor + 1), &fds, NULL, NULL, NULL);

    if (selectResult < 0)
    {
        return SHUResult_ErrNetwork;
    }

    return SHU_ListenerCheck(listener, retClientConnection);
}

SHUResult SHU_ConnectionSend(SHUConnection *connection, const char *data, usz dataSize, usz *retSentSize)
{
    SHU_AssertNullPointer(connection);
    SHU_AssertNullPointer(data);
    SHUI_CheckPanicConnection(connection);

    int bytesSent = send(connection->fileDescriptor, data, (int)dataSize, 0);

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

    if (retSentSize != NULL)
    {
        *retSentSize = bytesSent;
    }

    return SHUResult_Ok;
}

SHUResult SHU_ConnectionSendWait(SHUConnection *connection, const char *data, usz dataSize)
{
    SHU_AssertNullPointer(connection);
    SHU_AssertNullPointer(data);
    SHUI_CheckPanicConnection(connection);

    usz totalSent = 0;

    while (totalSent < dataSize)
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(connection->fileDescriptor, &fds);

        int selectResult = select((int)(connection->fileDescriptor + 1), NULL, &fds, NULL, NULL);

        if (selectResult < 0)
        {
            return SHUResult_ErrNetwork;
        }

        int bytesSent = send(connection->fileDescriptor, data + totalSent, (int)(dataSize - totalSent), 0);

        if (bytesSent < 0)
        {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEWOULDBLOCK)
            {
                continue;
            }
#else
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                continue;
            }
#endif
            return SHUResult_ErrNetwork;
        }

        totalSent += (usz)bytesSent;
    }

    return SHUResult_Ok;
}

SHUResult SHU_ConnectionReceive(SHUConnection *connection, char *buffer, usz bufferSize, usz *retReceivedSize)
{
    SHU_AssertNullPointer(connection);
    SHU_AssertNullPointer(buffer);
    SHUI_CheckPanicConnection(connection);

    int bytesReceived = recv(connection->fileDescriptor, buffer, (int)bufferSize, 0);

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

    if (retReceivedSize != NULL)
    {
        *retReceivedSize = (usz)bytesReceived;
    }

    return SHUResult_Ok;
}

SHUResult SHU_ConnectionReceiveWait(SHUConnection *connection, char *buffer, usz bufferSize)
{
    SHU_AssertNullPointer(connection);
    SHU_AssertNullPointer(buffer);
    SHUI_CheckPanicConnection(connection);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(connection->fileDescriptor, &fds);

    int selectResult = select((int)(connection->fileDescriptor + 1), &fds, NULL, NULL, NULL);

    if (selectResult < 0)
    {
        return SHUResult_ErrNetwork;
    }

    return SHU_ConnectionReceive(connection, buffer, bufferSize, NULL);
}

#endif

#pragma endregion Definitions
