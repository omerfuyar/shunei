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

/// @brief Resolves a hostname to its first IPv4 address, as a dotted-decimal string usable with SHU_ConnectionCreate.
/// @param hostname Hostname to resolve, e.g. "google.com". Must not be NULL.
/// @param retIp Buffer that receives the resolved address.
/// @return Result of the operation. ErrNetwork if the hostname could not be resolved, ErrOverflow if retIpCapacity is too small. See SHUResult for details.
SHUResult SHU_ResolveDNS(const char *hostname, SHUSlice retIp);

/// @brief Creates and configures a client connection to the specified ip and port.
/// @param retConnection Pointer to a SHUConnection struct where the created connection information will be stored. Must not be NULL.
/// @param ip IP address to connect to. Must not be NULL.
/// @param port Port number to connect to.
/// @return Result of the operation. See SHUResult for details.
SHUResult SHU_ConnectionCreate(SHUConnection *retConnection, const char *ip, u16 port);

/// @brief Creates and configures a listener connection with the specified parameters.
/// @param retListener Pointer to a SHUListener struct where the created listener information will be stored. Must not be NULL.
/// @param ip IP address to bind to. Can be NULL to bind to all interfaces.
/// @param port Port number to bind to.
/// @param clientConnectionsBuffer Buffer to store client connections. Cannot be NULL.
/// @param maxClientConnections Maximum number of client connections active at the same time. Slots freed with SHU_ListenerReleaseClient can be reused.
/// @return Result of the operation. See SHUResult for details.
SHUResult SHU_ListenerCreate(SHUListener *retListener, const char *ip, u16 port, SHUConnection *clientConnectionsBuffer, usz maxClientConnections);

/// @brief Destroys a connection and releases any associated resources.
/// @param connection Pointer to the SHUConnection to be destroyed.
/// @return Result of the operation. See SHUResult for details.
SHUResult SHU_ConnectionDestroy(SHUConnection *connection);

/// @brief Destroys a listener connection and releases any associated resources.
/// @param listener Pointer to the SHUListener to be destroyed.
/// @return Result of the operation. See SHUResult for details.
/// @note Does not destroy any client connections previously accepted through this listener. Release those separately with SHU_ListenerReleaseClient first.
SHUResult SHU_ListenerDestroy(SHUListener *listener);

/// @brief Checks for a waiting client in a listener.
/// @param listener Listener to check for waiting clients.
/// @param retClientConnection pointer to the client connection that is connected to this listener if so. Must not be NULL.
/// @return Result of the operation. Pending if there is no connecting clients, Ok if a client was accepted, ErrIndexOutOfBounds if the listener's client buffer is already full. See SHUResult for details.
SHUWUR SHUResult SHU_ListenerCheck(SHUListener *listener, SHUConnection **retClientConnection);

/// @brief Waits for a client to connect.
/// @param listener Listener to wait for clients.
/// @param retClientConnection pointer to the client connection that is connected to this listener if so. Must not be NULL.
/// @return Result of the operation. Ok once a client has been accepted, ErrIndexOutOfBounds if the listener's client buffer is already full. See SHUResult for details.
SHUResult SHU_ListenerWait(SHUListener *listener, SHUConnection **retClientConnection);

/// @brief Releases a client connection previously accepted through SHU_ListenerCheck or SHU_ListenerWait, closing it and freeing its slot for reuse by a future accept.
/// @param listener Listener that accepted connection. Must not be NULL.
/// @param connection Client connection to release. Must have been accepted through this listener. Must not be NULL.
/// @return Result of the operation. See SHUResult for details.
SHUResult SHU_ListenerReleaseClient(SHUListener *listener, SHUConnection *connection);

/// @brief Attempts to send data through a connection without blocking.
/// @param connection Pointer to the SHUConnection struct representing the connection to send data through. Must not be NULL.
/// @param data Slice view of the data to send. data.data must not be NULL.
/// @param retSentSize Bytes actually sent by this call, which may be less than data.size. Leave NULL if not needed.
/// @return Result of the operation. Pending if the socket cannot accept any data right now, Ok if at least one byte was sent. See SHUResult for details.
/// @note It may only send part of data.size. Check retSentSize even on SHUResult_Ok and call again with the remaining data (csv(cs((u8*)data.data + *retSentSize, data.size - *retSentSize))) until it has all gone out.
SHUWUR SHUResult SHU_ConnectionSendSplit(SHUConnection *connection, SHUSliceView data, usz *retSentSize);

/// @brief Sends data through a connection, blocking until all of it has been sent.
/// @param connection Pointer to the SHUConnection struct representing the connection to send data through. Must not be NULL.
/// @param data Slice view of the data to send. data.data must not be NULL.
/// @param retSentSize Bytes actually sent. Leave NULL if not needed.
/// @return Result of the operation. See SHUResult for details.
SHUResult SHU_ConnectionSendWait(SHUConnection *connection, SHUSliceView data, usz *retSentSize);

/// @brief Attempts to receive data through a connection without blocking.
/// @param connection Pointer to the SHUConnection struct representing the connection to receive data from. Must not be NULL.
/// @param buffer Slice of the buffer where received data will be stored. buffer.data must not be NULL.
/// @param retReceivedSize Bytes actually received by this call, which may be less than buffer.size. Leave NULL if not needed.
/// @return Result of the operation. Pending if there is no data to receive yet, Ok if at least one byte was received, Finished if the peer closed the connection. See SHUResult for details.
/// @note It may only fill part of buffer.size. Check retReceivedSize on SHUResult_Ok and call again to receive the rest.
SHUWUR SHUResult SHU_ConnectionReceiveSplit(SHUConnection *connection, SHUSlice buffer, usz *retReceivedSize);

/// @brief Receives data through a connection, blocking until the buffer is completely filled or the
/// peer closes the connection, whichever happens first.
/// @param connection Pointer to the SHUConnection struct representing the connection to receive data from. Must not be NULL.
/// @param buffer Slice of the buffer where received data will be stored. buffer.data must not be NULL.
/// @param retReceivedSize Bytes actually placed in the buffer. Leave NULL if not needed.
/// @return Result of the operation. Ok once at least one byte has been received, Finished if the connection was already closed before any data arrived. See SHUResult for details.
SHUResult SHU_ConnectionReceiveWait(SHUConnection *connection, SHUSlice buffer, usz *retReceivedSize);

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
#include <netdb.h>
#define SHUI_CheckSocket(socket) (socket >= 0)
#endif

// send() must never be allowed to raise SIGPIPE on a broken connection; it should report
// SHUResult_ErrNetwork like any other socket error instead. MSG_NOSIGNAL (Linux) and
// SO_NOSIGPIPE (macOS/BSD, applied per-socket in SHUI_DisableSigPipe) cover this without
// touching process-wide signal disposition. Neither exists on Windows, which has no SIGPIPE.
#ifdef MSG_NOSIGNAL
#define SHUI_SEND_FLAGS MSG_NOSIGNAL
#else
#define SHUI_SEND_FLAGS 0
#endif

#define SHUI_AssertConnection(connection) SHU_Assert(SHUI_CheckSocket((connection)->fileDescriptor) &&  \
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

/// @brief Opts connection's socket out of raising SIGPIPE on a broken pipe, where the platform supports it. No-op elsewhere.
static void SHUI_DisableSigPipe(SHUConnection *connection)
{
#ifdef SO_NOSIGPIPE
    i32 noSigPipe = 1;
    setsockopt(connection->fileDescriptor, SOL_SOCKET, SO_NOSIGPIPE, (const char *)&noSigPipe, sizeof(noSigPipe));
#else
    (void)connection;
#endif
}

#ifdef _WIN32
/// @brief Windows-only: send()/recv() on a socket whose non-blocking connect() hasn't finished yet report
/// WSAENOTCONN, identically whether the connect is still in progress or has already failed -- unlike POSIX,
/// where this case is already covered by EWOULDBLOCK/EAGAIN. SO_ERROR disambiguates: still 0 means the
/// connect genuinely hasn't resolved yet, nonzero means it has and this really is a failure.
static bool SHUI_IsStillConnecting(SHUConnection *connection)
{
    i32 soError = 0;
    i32 soErrorLen = sizeof(soError);
    getsockopt(connection->fileDescriptor, SOL_SOCKET, SO_ERROR, (char *)&soError, &soErrorLen);
    return soError == 0;
}
#endif

/// @brief Blocks until connection's socket is ready for the requested direction. Shared by every *Wait function.
static SHUResult SHUI_WaitForSocket(SHUConnection *connection, bool forWrite)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(connection->fileDescriptor, &fds);

    i32 selectResult = select((int)(connection->fileDescriptor + 1), forWrite ? NULL : &fds, forWrite ? &fds : NULL, NULL, NULL);

    if (selectResult < 0)
    {
        return SHUResult_ErrNetwork;
    }

    return SHUResult_Ok;
}

#pragma endregion Internals

SHUResult SHU_InitializeNetwork(void)
{
    SHUNEI.atExitFunction = SHU_TerminateNetwork;
    atexit(SHUNEI_AT_EXIT);

#ifdef _WIN32
    WSADATA wsa;
    i32 result = WSAStartup(MAKEWORD(2, 2), &wsa);
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
    i32 result = WSACleanup();
    if (result)
    {
        return SHUResult_ErrNetwork;
    }
#endif

    return SHUResult_Ok;
}

SHUResult SHU_ResolveDNS(const char *hostname, SHUSlice retIp)
{
    SHU_AssertNullPointer(hostname);
    SHU_AssertSlice(retIp);

    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *resolved = NULL;
    if (getaddrinfo(hostname, NULL, &hints, &resolved) != 0)
    {
        return SHUResult_ErrNetwork;
    }

    const char *converted = inet_ntop(AF_INET, &((struct sockaddr_in *)resolved->ai_addr)->sin_addr, retIp.data, retIp.size);

    freeaddrinfo(resolved);

    if (converted == NULL)
    {
        return SHUResult_ErrOverflow;
    }

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

    SHUI_DisableSigPipe(retConnection);

    memset(&retConnection->address, 0, sizeof(retConnection->address));
    retConnection->address.sin_family = AF_INET;
    retConnection->address.sin_port = htons(port);
    retConnection->addressLength = sizeof(struct sockaddr_in);

    if (inet_pton(AF_INET, ip, &retConnection->address.sin_addr.s_addr) != 1)
    {
        return SHUResult_ErrNetwork;
    }

#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(retConnection->fileDescriptor, (long)FIONBIO, &mode);
#else
    i32 flags = fcntl(retConnection->fileDescriptor, F_GETFL, 0);
    fcntl(retConnection->fileDescriptor, F_SETFL, flags | O_NONBLOCK);
#endif

    if (connect(retConnection->fileDescriptor, (struct sockaddr *)&retConnection->address, sizeof(struct sockaddr_in)) < 0)
    {
        // a non-blocking connect is expected to report "in progress" here; anything else is a real, immediate failure
#ifdef _WIN32
        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            return SHUResult_ErrNetwork;
        }
#else
        if (errno != EINPROGRESS)
        {
            return SHUResult_ErrNetwork;
        }
#endif
    }

    return SHUResult_Ok;
}

SHUResult SHU_ListenerCreate(SHUListener *retListener, const char *ip, u16 port, SHUConnection *clientConnectionsBuffer, usz maxClientConnections)
{
    SHU_AssertNullPointer(retListener);
    SHU_AssertNullPointer(clientConnectionsBuffer);

    retListener->clientConnections = clientConnectionsBuffer;
    retListener->clientCapacity = maxClientConnections;
    retListener->clientCount = 0;

    memset(clientConnectionsBuffer, 0x00, maxClientConnections * sizeof(SHUConnection));

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
    i32 flags = fcntl(retListener->connection.fileDescriptor, F_GETFL, 0);
    fcntl(retListener->connection.fileDescriptor, F_SETFL, flags | O_NONBLOCK);
#endif

    i32 opt = 1;
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
    SHUI_AssertConnection(connection);

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
    SHU_AssertNullPointer(listener);
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
    SHUI_AssertConnection(&listener->connection);

    if (listener->clientCount >= listener->clientCapacity)
    {
        return SHUResult_ErrIndexOutOfBounds;
    }

    SHUConnection *connection = NULL;

    for (usz i = 0; i < listener->clientCapacity; i++)
    {
        if (listener->clientConnections[i].address.sin_port == 0)
        {
            connection = listener->clientConnections + i;
            break;
        }
    }

    SHU_Assert(connection != NULL, "Listener has room (clientCount < clientCapacity) but no free slot was found in clientConnections.");

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

    SHUI_DisableSigPipe(connection);

    *retClientConnection = connection;
    listener->clientCount++;

    return SHUResult_Ok;
}

SHUResult SHU_ListenerWait(SHUListener *listener, SHUConnection **retClientConnection)
{
    SHU_AssertNullPointer(listener);
    SHU_AssertNullPointer(retClientConnection);
    SHUI_AssertConnection(&listener->connection);

    for (;;)
    {
        SHU_CheckReturn(SHUI_WaitForSocket(&listener->connection, false));

        SHUResult result = SHU_ListenerCheck(listener, retClientConnection);

        if (result == SHUResult_Pending)
        {
            continue;
        }

        return result;
    }
}

SHUResult SHU_ListenerReleaseClient(SHUListener *listener, SHUConnection *connection)
{
    SHU_AssertNullPointer(listener);
    SHU_AssertNullPointer(connection);
    SHU_Assert(connection >= listener->clientConnections && connection < listener->clientConnections + listener->clientCapacity,
               "Connection was not accepted through this listener.");

    SHU_CheckReturn(SHU_ConnectionDestroy(connection));

    listener->clientCount--;

    return SHUResult_Ok;
}

SHUResult SHU_ConnectionSendSplit(SHUConnection *connection, SHUSliceView data, usz *retSentSize)
{
    SHU_AssertNullPointer(connection);
    SHU_AssertNullPointer(data.data);
    SHUI_AssertConnection(connection);

    i32 bytesSent = send(connection->fileDescriptor, data.data, (int)data.size, SHUI_SEND_FLAGS);

    if (bytesSent < 0)
    {
#ifdef _WIN32
        i32 wsaError = WSAGetLastError();
        if (wsaError == WSAEWOULDBLOCK || (wsaError == WSAENOTCONN && SHUI_IsStillConnecting(connection)))
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
        *retSentSize = (usz)bytesSent;
    }

    return SHUResult_Ok;
}

SHUResult SHU_ConnectionSendWait(SHUConnection *connection, SHUSliceView data, usz *retSentSize)
{
    SHU_AssertNullPointer(connection);
    SHU_AssertNullPointer(data.data);
    SHUI_AssertConnection(connection);

    usz totalSent = 0;

    while (totalSent < data.size)
    {
        SHU_CheckReturn(SHUI_WaitForSocket(connection, true));

        usz sent = 0;
        SHUSliceView remaining = csv(cs((void *)((const u8 *)data.data + totalSent), data.size - totalSent));
        SHUResult result = SHU_ConnectionSendSplit(connection, remaining, &sent);

        if (result == SHUResult_Pending)
        {
            continue;
        }

        SHU_CheckReturn(result);

        totalSent += sent;
    }

    if (retSentSize != NULL)
    {
        *retSentSize = totalSent;
    }

    return SHUResult_Ok;
}

SHUResult SHU_ConnectionReceiveSplit(SHUConnection *connection, SHUSlice buffer, usz *retReceivedSize)
{
    SHU_AssertNullPointer(connection);
    SHU_AssertNullPointer(buffer.data);
    SHUI_AssertConnection(connection);

    i32 bytesReceived = recv(connection->fileDescriptor, buffer.data, (int)buffer.size, 0);

    if (bytesReceived < 0)
    {
#ifdef _WIN32
        i32 wsaError = WSAGetLastError();
        if (wsaError == WSAEWOULDBLOCK || (wsaError == WSAENOTCONN && SHUI_IsStillConnecting(connection)))
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

    if (bytesReceived == 0)
    {
        return SHUResult_Finished; // peer closed the connection
    }

    if (retReceivedSize != NULL)
    {
        *retReceivedSize = (usz)bytesReceived;
    }

    return SHUResult_Ok;
}

SHUResult SHU_ConnectionReceiveWait(SHUConnection *connection, SHUSlice buffer, usz *retReceivedSize)
{
    SHU_AssertNullPointer(connection);
    SHU_AssertSlice(buffer);
    SHUI_AssertConnection(connection);

    usz totalReceived = 0;

    while (totalReceived < buffer.size)
    {
        SHU_CheckReturn(SHUI_WaitForSocket(connection, false));

        usz received = 0;
        SHUSlice remaining = cs((u8 *)buffer.data + totalReceived, buffer.size - totalReceived);
        SHUResult result = SHU_ConnectionReceiveSplit(connection, remaining, &received);

        if (result == SHUResult_Pending)
        {
            continue;
        }

        if (result == SHUResult_Finished)
        {
            break; // peer closed the connection before the buffer was filled
        }

        SHU_CheckReturn(result);

        totalReceived += received;
    }

    if (retReceivedSize != NULL)
    {
        *retReceivedSize = totalReceived;
    }

    return totalReceived > 0 ? SHUResult_Ok : SHUResult_Finished;
}

#endif

#pragma endregion Definitions
