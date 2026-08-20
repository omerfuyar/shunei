#define SHU_IMPLEMENTATION
#include "../shunei.h"

int main(void)
{
    // init before using any functions
    SHU_CheckPanic(SHU_InitializeNetwork());

    // listener - server
    SHUConnection clientSlots[4];
    SHUListener server;
    SHU_CheckPanic(SHU_ListenerCreate(&server, "127.0.0.1", 7000, clientSlots, 4));

    // connection - client
    SHUConnection client;
    SHU_CheckPanic(SHU_ConnectionCreate(&client, "127.0.0.1", 7000));

    SHU_LogInfo("Waiting for the client to connect...");

    // blocks until a client connects
    SHUConnection *serverSideClient = NULL;
    SHU_CheckPanic(SHU_ListenerWait(&server, &serverSideClient));

    SHU_LogInfo("Client connected, sending message");

    // client sends, blocking until it's all gone out
    const char *message = "hello from client";
    usz messageSize = strlen(message) + 1;
    SHU_CheckPanic(SHU_ConnectionSendWait(&client, csv(cs((void *)message, messageSize)), NULL));

    // server receives, blocking until the buffer is full
    char buffer[128] = {0};
    usz receivedSize = 0;
    SHU_CheckPanic(SHU_ConnectionReceiveWait(serverSideClient, cs(buffer, messageSize), &receivedSize));

    SHU_LogInfo("Server received %zu out of %zu bytes: %s", receivedSize, messageSize, buffer);

    // cleanup
    SHU_CheckPanic(SHU_ListenerReleaseClient(&server, serverSideClient));
    SHU_CheckPanic(SHU_ConnectionDestroy(&client));
    SHU_CheckPanic(SHU_ListenerDestroy(&server));
    SHU_CheckPanic(SHU_TerminateNetwork());

    return 0;
}
