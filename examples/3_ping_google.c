#define SHU_IMPLEMENTATION
#include "../shunei.h"

#ifndef _WIN32
#include <netdb.h>
#endif

int main(void)
{
    SHU_CheckPanic(SHU_InitializeNetwork());

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *resolved = NULL;
    if (getaddrinfo("google.com", "80", &hints, &resolved) != 0)
    {
        SHU_LogError(SHUResult_ErrNetwork, "failed to resolve google.com");
        return 1;
    }

    char ip[SHUM_ADDRESS_STRLEN];
    inet_ntop(AF_INET, &((struct sockaddr_in *)resolved->ai_addr)->sin_addr, ip, sizeof(ip));
    freeaddrinfo(resolved);

    SHU_LogInfo("resolved google.com to %s", ip);

    SHUConnection connection;
    SHU_CheckPanic(SHU_ConnectionCreate(&connection, ip, 80));

    const char *request = "GET / HTTP/1.1\r\nHost: google.com\r\nConnection: close\r\n\r\n";
    SHU_CheckPanic(SHU_ConnectionSendWait(&connection, request, strlen(request)));

    SHU_LogInfo("request sent, waiting for response...");

    char buffer[512];
    usz receivedSize;
    usz totalReceived = 0;
    SHUResult result;

    for (;;)
    {
        while ((result = SHU_ConnectionReceive(&connection, buffer, sizeof(buffer) - 1, &receivedSize)) == SHUResult_Pending)
        {
        }
        SHU_CheckPanic(result);

        if (receivedSize == 0)
        {
            break; // google closed the connection, response is complete
        }

        buffer[receivedSize] = '\0';
        totalReceived += receivedSize;

        if (totalReceived <= sizeof(buffer)) // only print the first chunk, the rest is body we don't care about here
        {
            SHU_LogInfo("first bytes of response:\n%s", buffer);
        }
    }

    SHU_LogInfo("connection closed by google, received %zu bytes total", totalReceived);

    SHU_CheckPanic(SHU_ConnectionDestroy(&connection));
    SHU_CheckPanic(SHU_TerminateNetwork());

    return 0;
}
