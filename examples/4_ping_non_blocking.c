#define SHU_IMPLEMENTATION
#include "../shunei.h"

int main(void)
{
    SHU_CheckPanic(SHU_InitializeNetwork());

    // resolve google.com to an ip, shunei only takes ips
    char ip[SHUM_ADDRESS_STRLEN];
    SHU_CheckPanic(SHU_ResolveDNS("google.com", cs(ip, sizeof(ip))));

    SHU_LogInfo("resolved google.com to %s", ip);

    // connection - client
    SHUConnection connection;
    SHU_CheckPanic(SHU_ConnectionCreate(&connection, ip, 80));

    // send a plain http request, non-blocking
    const char *request = "GET / HTTP/1.1\r\nHost: google.com\r\nConnection: close\r\n\r\n";
    usz requestSize = strlen(request);
    usz sentSize = 0;

    while (sentSize < requestSize)
    {
        usz sent = 0;
        SHUSlice dataToSend = cs((void *)(request + sentSize), requestSize - sentSize);
        SHUResult sendResult = SHU_ConnectionSendSplit(&connection, csv(dataToSend), &sent);

        if (sendResult == SHUResult_Ok)
        {
            sentSize += sent;
        }
        else if (sendResult != SHUResult_Pending)
        {
            SHU_CheckPanic(sendResult);
        }
    }

    SHU_LogInfo("request sent, waiting for response...");

    char buffer[512];
    usz receivedSize;
    usz totalReceived = 0;
    SHUResult result;

    for (usz passCount = 0;; passCount++)
    {
        while ((result = SHU_ConnectionReceiveSplit(&connection, cs(buffer, sizeof(buffer) - 1), &receivedSize)) == SHUResult_Pending)
        {
            // do work
        }

        if (result == SHUResult_Finished)
        {
            break; // google closed the connection, response is complete
        }

        SHU_CheckPanic(result);

        buffer[receivedSize] = '\0';
        totalReceived += receivedSize;

        SHU_LogInfo("pass %zu data %zu bytes :\n'%s'", passCount, receivedSize, buffer);
    }

    SHU_LogInfo("connection closed by google, received %zu bytes total", totalReceived);

    // cleanup
    SHU_CheckPanic(SHU_ConnectionDestroy(&connection));
    SHU_CheckPanic(SHU_TerminateNetwork());

    return 0;
}
