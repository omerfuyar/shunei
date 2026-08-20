#define SHU_IMPLEMENTATION
#include "../shunei.h"

int main(void)
{
    SHU_CheckPanic(SHU_InitializeNetwork());

    // resolve google.com to an ip, shunei only takes ips
    char ip[SHUM_ADDRESS_STRLEN];
    SHU_CheckPanic(SHU_DnsResolve("google.com", cs(ip, sizeof(ip))));

    SHU_LogInfo("resolved google.com to %s", ip);

    // connection - client
    SHUConnection connection;
    SHU_CheckPanic(SHU_ConnectionCreate(&connection, ip, 80));

    // send a plain http request
    const char *request = "GET / HTTP/1.1\r\nHost: google.com\r\nConnection: close\r\n\r\n";
    SHU_CheckPanic(SHU_ConnectionSendWait(&connection, csv(cs((void *)request, strlen(request))), NULL));

    SHU_LogInfo("request sent, waiting for response...");

    char buffer[512];
    usz receivedSize;
    usz totalReceived = 0;
    SHUResult result;

    for (;;)
    {
        while ((result = SHU_ConnectionReceiveSplit(&connection, cs(buffer, sizeof(buffer) - 1), &receivedSize)) == SHUResult_Pending)
        {
        }

        if (result == SHUResult_Err)
        {
            break; // google closed the connection, response is complete
        }

        SHU_CheckPanic(result);

        buffer[receivedSize] = '\0';
        totalReceived += receivedSize;

        if (totalReceived <= sizeof(buffer)) // only print the first chunk, the rest is body we don't care about here
        {
            SHU_LogInfo("first bytes of response:\n%s", buffer);
        }
    }

    SHU_LogInfo("connection closed by google, received %zu bytes total", totalReceived);

    // cleanup
    SHU_CheckPanic(SHU_ConnectionDestroy(&connection));
    SHU_CheckPanic(SHU_TerminateNetwork());

    return 0;
}
