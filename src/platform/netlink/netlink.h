#ifndef NETLINK_H
#define NETLINK_H
#include <stddef.h>

// Returns "sa1" or "sa2" based on the compile-time GAME constant.
const char *NetLink_GetGameId(void);

// Connect to the relay and either create or join a named room.
// Returns 1 if the request was sent, 0 on immediate failure (e.g. not
// yet initialized). Actual success/failure arrives later via
// NetLink_GetAssignedId() becoming valid, or the pending-start flag.
int NetLink_Connect(const char *room_name, int as_host);

// Our own assigned player slot once paired: 0 = host/parent, 1-3 = joiner.
// Returns -1 if not yet assigned.
int NetLink_GetAssignedId(void);

// Send our 20-byte payload to every other player in the room.
void NetLink_Send(const unsigned char *data, size_t len);

// Non-blocking: if a payload arrived from player slot `player_index`
// since the last poll, copy up to `max_len` bytes into out_buf and
// return 1. Otherwise return 0 immediately.
int NetLink_PollSlot(int player_index, unsigned char *out_buf, size_t max_len);

void NetLink_Disconnect(void);

// Opens the native host/join dialog (Android: GameActivity's AlertDialog).
// Called from the multiplayer button's tap handler in sdl2.c.
void OpenMultiplayerMenu(void);

// Call once per frame from the main loop. Returns 1 exactly once, the
// first frame after the relay has assigned us a player id — the signal
// to call MultiSioInit()/StartMultiPakConnect() and hand off into the
// game's own waiting-room screen. Returns 0 every other frame.
int NetLink_ConsumePendingMultiplayerStart(void);

#endif
