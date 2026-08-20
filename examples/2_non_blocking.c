#define SHU_IMPLEMENTATION
#include "../shunei.h"

int main(void)
{
    SHU_CheckPanic(SHU_InitializeNetwork());

    SHUConnection clientSlots[4];
    SHUListener server;
    SHU_CheckPanic(SHU_ListenerCreate(&server, "127.0.0.1", 7001, clientSlots, 4));

    SHUConnection client;
    SHU_CheckPanic(SHU_ConnectionCreate(&client, "127.0.0.1", 7001));

    SHU_LogInfo("polling for the client to connect...");

    SHUConnection *serverSide = NULL;
    SHUResult result;
    usz checks = 0;

    while ((result = SHU_ListenerCheck(&server, &serverSide)) == SHUResult_Pending)
    {
        checks++;
    }
    SHU_CheckPanic(result);

    SHU_LogInfo("client connected after %zu checks", checks);

    const char *message = "hello from client";
    usz messageSize = strlen(message) + 1;
    usz sentSize = 0;
    checks = 0;

    // SHU_ConnectionSend does not loop until everything is sent, so it can only be used
    // like this for messages small enough to always send in one call. For anything larger,
    // or when the byte count matters, use SHU_ConnectionSendWait instead.
    while ((result = SHU_ConnectionSend(&client, message, messageSize, &sentSize)) == SHUResult_Pending)
    {
        checks++;
    }
    SHU_CheckPanic(result);

    SHU_LogInfo("message sent after %zu checks, %zu of %zu bytes", checks, sentSize, messageSize);

    char buffer[128] = {0};
    usz receivedSize = 0;
    checks = 0;

    while ((result = SHU_ConnectionReceive(serverSide, buffer, sizeof(buffer), &receivedSize)) == SHUResult_Pending)
    {
        checks++;
    }
    SHU_CheckPanic(result);

    SHU_LogInfo("server received after %zu checks, %zu bytes: %s", checks, receivedSize, buffer);

    SHU_CheckPanic(SHU_ConnectionDestroy(serverSide));
    SHU_CheckPanic(SHU_ConnectionDestroy(&client));
    SHU_CheckPanic(SHU_ListenerDestroy(&server));
    SHU_CheckPanic(SHU_TerminateNetwork());

    return 0;
}
