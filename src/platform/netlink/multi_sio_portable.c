// PORTABLE replacement for multi_sio.c: same public API, backed by NetLink
// instead of GBA SIO hardware. Compiled only for android/emscripten.

#include "global.h"
#include "core.h"
#include "multi_sio.h"
#include "netlink.h"
#include <string.h>

struct MultiSioArea gMultiSioArea = {};

// Hardware delivers every player's packet every frame; a WebSocket doesn't.
// A peer stays "connected" until this many frames pass with nothing from it.
#define SLOT_TIMEOUT_FRAMES 45
#define SLOT_NEVER_RECEIVED 255

// What SIOMULTI0-3 read for a slot with nothing plugged in. 0 would mean
// "no transfer finished yet", which the connect screen treats as "keep waiting".
#define SIOMULTI_EMPTY 0xFFFF

static u8 sFramesSinceRecv[MULTI_SIO_PLAYERS_MAX] = {
    SLOT_NEVER_RECEIVED, SLOT_NEVER_RECEIVED, SLOT_NEVER_RECEIVED, SLOT_NEVER_RECEIVED
};
static u8 sSessionRan = 0;

// 0 = parent/host, 1-3 = child. Unassigned acts like a lone GBA: parent, id 0.
static u8 GetMyPlayerId(void)
{
    int id = NetLink_GetAssignedId();
    return (id >= 0 && id < MULTI_SIO_PLAYERS_MAX) ? (u8)id : 0;
}

void MultiSioInit(u32 connectedFlags)
{
    u8 i;

    CpuFill32(0, &gMultiSioArea, sizeof(struct MultiSioArea));
    gMultiSioArea.connectedFlags = (u8)connectedFlags;
    gMultiSioArea.type = (GetMyPlayerId() == 0) ? SIO_MULTI_PARENT : SIO_MULTI_CHILD;
    for (i = 0; i < MULTI_SIO_PLAYERS_MAX; i++)
        sFramesSinceRecv[i] = SLOT_NEVER_RECEIVED;
}

void MultiSioStart(void)
{
    if (gMultiSioArea.type)
        gMultiSioArea.startFlag = 1;
}

// Every "leave multiplayer" path clears gMultiSioEnabled before calling this.
// The routine reset at mode select happens before a session starts, so it
// must not drop the connection.
void MultiSioStop(void)
{
    if (!gMultiSioEnabled && sSessionRan) {
        NetLink_Disconnect();
        sSessionRan = 0;
    }
    gMultiSioArea.startFlag = 0;
}

u32 MultiSioMain(void *sendp, void *recvp, u32 loadRequest)
{
    u8 myId = GetMyPlayerId();
    u8 recvSuccessFlags = 0;
    u8 i;

    sSessionRan = 1;

    // Re-derive the role every frame so it doesn't matter when the id arrived.
    gMultiSioArea.type = (myId == 0) ? SIO_MULTI_PARENT : SIO_MULTI_CHILD;

    NetLink_Send((const unsigned char *)sendp, MULTI_SIO_BLOCK_SIZE);

    for (i = 0; i < MULTI_SIO_PLAYERS_MAX; i++) {
        unsigned char *slot = (unsigned char *)recvp + i * MULTI_SIO_BLOCK_SIZE;

        if (i == myId) {
            // Hardware echoes your own transmission into your own slot.
            memcpy(slot, sendp, MULTI_SIO_BLOCK_SIZE);
            recvSuccessFlags |= (1 << i);
            continue;
        }

        // On a miss the slot keeps its last data, as on hardware.
        if (NetLink_PollSlot(i, slot, MULTI_SIO_BLOCK_SIZE))
            sFramesSinceRecv[i] = 0;
        else if (sFramesSinceRecv[i] < SLOT_NEVER_RECEIVED)
            sFramesSinceRecv[i]++;

        if (sFramesSinceRecv[i] < SLOT_TIMEOUT_FRAMES)
            recvSuccessFlags |= (1 << i);
    }

    gMultiSioArea.recvSuccessFlags = recvSuccessFlags;
    gMultiSioArea.connectedFlags |= recvSuccessFlags;

    ((struct SioMultiCnt *)REG_ADDR_SIOCNT)->id = myId;
    for (i = 0; i < MULTI_SIO_PLAYERS_MAX; i++)
        (&REG_SIOMULTI0)[i] = (recvSuccessFlags & (1 << i)) ? MULTI_SIO_SYNC_DATA : SIOMULTI_EMPTY;

    // Multiboot download readiness has no equivalent here (everyone has the
    // full game), so treat it as done once the link is up.
    if (recvSuccessFlags & 1) {
        if (gMultiSioArea.type == SIO_MULTI_PARENT) {
            if ((recvSuccessFlags & 0x3) && recvSuccessFlags == gMultiSioArea.connectedFlags)
                gMultiSioArea.loadEnable = 1;
            if (gMultiSioArea.loadEnable)
                gMultiSioArea.loadSuccessFlag = 1;
        } else {
            gMultiSioArea.loadSuccessFlag = 1;
        }
    }

    gMultiSioArea.loadRequest |= loadRequest;
    ++gMultiSioArea.sendFrameCounter;

    return gMultiSioArea.recvSuccessFlags
         | gMultiSioArea.loadEnable << 4
         | gMultiSioArea.loadRequest << 5
         | gMultiSioArea.loadSuccessFlag << 6
         | (gMultiSioArea.type == SIO_MULTI_PARENT) << 7
         | gMultiSioArea.connectedFlags << 8
         | (gMultiSioArea.hardError != 0) << 12;
}

void MultiSioIntr(void)
{
}
