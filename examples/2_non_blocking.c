#define SHU_IMPLEMENTATION
#include "../shunei.h"

#define SHUM_PAYLOAD_SIZE (4 * 1024 * 1024)
#define SHUM_CHUNK_SIZE 4096

int main(void)
{
    SHU_CheckPanic(SHU_InitializeNetwork());

    SHUConnection clientSlots[4];
    SHUListener server;
    SHU_CheckPanic(SHU_ListenerCreate(&server, "127.0.0.1", 7001, clientSlots, 4));

    SHUConnection client;
    SHU_CheckPanic(SHU_ConnectionCreate(&client, "127.0.0.1", 7001));

    SHUConnection *serverSide = NULL;
    while (SHU_ListenerCheck(&server, &serverSide) == SHUResult_Pending)
    {
    }

    static char payload[SHUM_PAYLOAD_SIZE];
    for (usz i = 0; i < SHUM_PAYLOAD_SIZE; i++)
    {
        payload[i] = (char)(i % 256);
    }

    usz sendOffset = 0;
    usz receiveOffset = 0;
    usz sendFrames = 0;
    usz sendBatches = 0;
    usz receiveFrames = 0;
    usz receiveBatches = 0;

    char receiveChunk[SHUM_CHUNK_SIZE];

    // stands in for a game loop: every iteration is one "frame" that tries to make progress
    // on sending and receiving exactly once each, and moves on regardless of the result.
    while (sendOffset < SHUM_PAYLOAD_SIZE || receiveOffset < SHUM_PAYLOAD_SIZE)
    {
        if (sendOffset < SHUM_PAYLOAD_SIZE)
        {
            usz remaining = SHUM_PAYLOAD_SIZE - sendOffset;
            usz toSend = remaining < SHUM_CHUNK_SIZE ? remaining : SHUM_CHUNK_SIZE;

            usz sent = 0;
            SHUResult result = SHU_ConnectionSendSplit(&client, payload + sendOffset, toSend, &sent);
            sendFrames++;

            if (result == SHUResult_Ok)
            {
                sendOffset += sent;
                sendBatches++;
            }
            else if (result != SHUResult_Pending)
            {
                SHU_CheckPanic(result);
            }
        }

        if (receiveOffset < SHUM_PAYLOAD_SIZE)
        {
            usz received = 0;
            SHUResult result = SHU_ConnectionReceiveSplit(serverSide, receiveChunk, sizeof(receiveChunk), &received);
            receiveFrames++;

            if (result == SHUResult_Ok)
            {
                for (usz i = 0; i < received; i++)
                {
                    SHU_Assert(receiveChunk[i] == (char)((receiveOffset + i) % 256), "payload corrupted at byte %zu", receiveOffset + i);
                }

                receiveOffset += received;
                receiveBatches++;
            }
            else if (result != SHUResult_Pending)
            {
                SHU_CheckPanic(result);
            }
        }
    }

    SHU_LogInfo("sent %zu bytes over %zu frames, %zu of which made progress", sendOffset, sendFrames, sendBatches);
    SHU_LogInfo("received %zu bytes over %zu frames, %zu of which made progress", receiveOffset, receiveFrames, receiveBatches);
    SHU_LogInfo("payload verified byte-for-byte, no corruption across %zu receive batches", receiveBatches);

    SHU_CheckPanic(SHU_ConnectionDestroy(serverSide));
    SHU_CheckPanic(SHU_ConnectionDestroy(&client));
    SHU_CheckPanic(SHU_ListenerDestroy(&server));
    SHU_CheckPanic(SHU_TerminateNetwork());

    return 0;
}
