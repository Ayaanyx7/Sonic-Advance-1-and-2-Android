// src/platform/netlink/multi_sio_portable.c
// PORTABLE replacement for multi_sio.c — same public API, backed by
// NetLink (WebSocket relay) instead of real GBA SIO hardware timing.
// Compiled ONLY for android/emscripten; src/multi_sio.c is excluded
// from those platforms in the Makefile, so there's no duplicate symbol.

#include "global.h"
#include "multi_sio.h"
#include "netlink.h"

struct MultiSioArea gMultiSioArea = {};

static u8 sMyPlayerId = 0; // 0 = parent/host, 1-3 = child/joiner slot

void MultiSioInit(u32 connectedFlags)
{
    CpuFill32(0, &gMultiSioArea, sizeof(struct MultiSioArea));
    gMultiSioArea.connectedFlags = (u8)connectedFlags;
    sMyPlayerId = NetLink_GetAssignedId();
    gMultiSioArea.type = (sMyPlayerId == 0) ? SIO_MULTI_PARENT : SIO_MULTI_CHILD;
}

void MultiSioStart(void)
{
    if (gMultiSioArea.type)
        gMultiSioArea.startFlag = 1;
}

void MultiSioStop(void)
{
    NetLink_Disconnect();
    gMultiSioArea.startFlag = 0;
}

u32 MultiSioMain(void *sendp, void *recvp, u32 loadRequest)
{
    u8 recvSuccessFlags = 0;
    u8 i;

    NetLink_Send((const unsigned char *)sendp, MULTI_SIO_BLOCK_SIZE);

    for (i = 0; i < MULTI_SIO_PLAYERS_MAX; i++) {
        if (i == sMyPlayerId) {
            recvSuccessFlags |= (1 << i);
            continue;
        }
        if (NetLink_PollSlot(i, (unsigned char *)recvp + (i * MULTI_SIO_BLOCK_SIZE), MULTI_SIO_BLOCK_SIZE)) {
            recvSuccessFlags |= (1 << i);
        }
    }

    gMultiSioArea.recvSuccessFlags = recvSuccessFlags;
    gMultiSioArea.connectedFlags |= recvSuccessFlags;

    ((struct SioMultiCnt *)REG_ADDR_SIOCNT)->id = sMyPlayerId;
    for (i = 0; i < MULTI_SIO_PLAYERS_MAX; i++) {
        (&REG_SIOMULTI0)[i] = (recvSuccessFlags & (1 << i)) ? MULTI_SIO_SYNC_DATA : 0;
    }

    gMultiSioArea.loadRequest |= loadRequest;
    ++gMultiSioArea.sendFrameCounter;

    return gMultiSioArea.recvSuccessFlags
         | gMultiSioArea.loadEnable << 4
         | gMultiSioArea.loadRequest << 5
         | gMultiSioArea.loadSuccessFlag << 6
         | (gMultiSioArea.type == SIO_MULTI_PARENT) << 7
         | gMultiSioArea.connectedFlags << 8
         | (gMultiSioArea.hardError != 0) << 12
         | (sMyPlayerId >= MULTI_SIO_PLAYERS_MAX) << 13;
}
