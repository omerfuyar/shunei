#define SHU_IMPLEMENTATION
#include "../shunei.h"

int main(void)
{
    SHU_CheckPanic(SHU_InitializeNetwork());

    // resolve google.com to an ip
    char ip[SHUM_ADDRESS_STRLEN];
    SHU_CheckPanic(SHU_ResolveDNS("google.com", cs(ip, sizeof(ip))));

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

    for (usz passCount = 0;; passCount++)
    {
        // google sends 792 bytes:
        // first call 511 bytes received and printed
        // second call 281 bytes received and printed
        // third call is the finish call where google closes the connection
        result = SHU_ConnectionReceiveWait(&connection, cs(buffer, sizeof(buffer) - 1), &receivedSize);

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
