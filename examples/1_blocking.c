#define SHU_IMPLEMENTATION
#include "../shunei.h"

int main(void)
{
    SHU_CheckPanic(SHU_InitializeNetwork());

    SHUConnection clientSlots[4];
    SHUListener server;
    SHU_CheckPanic(SHU_ListenerCreate(&server, "127.0.0.1", 7000, clientSlots, 4));

    SHUConnection client;
    SHU_CheckPanic(SHU_ConnectionCreate(&client, "127.0.0.1", 7000));

    SHU_LogInfo("waiting for the client to connect...");

    SHUConnection *serverSide = NULL;
    SHU_CheckPanic(SHU_ListenerWait(&server, &serverSide));

    SHU_LogInfo("client connected, sending message");

    const char *message = "hello from client";
    SHU_CheckPanic(SHU_ConnectionSendWait(&client, message, strlen(message) + 1));

    char buffer[128] = {0};
    SHU_CheckPanic(SHU_ConnectionReceiveWait(serverSide, buffer, sizeof(buffer)));

    SHU_LogInfo("server received: %s", buffer);

    SHU_CheckPanic(SHU_ConnectionDestroy(serverSide));
    SHU_CheckPanic(SHU_ConnectionDestroy(&client));
    SHU_CheckPanic(SHU_ListenerDestroy(&server));
    SHU_CheckPanic(SHU_TerminateNetwork());

    return 0;
}
