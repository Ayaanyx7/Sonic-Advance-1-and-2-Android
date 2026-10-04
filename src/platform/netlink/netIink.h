#ifndef NETLINK_H
#define NETLINK_H
#include <stddef.h>

// Connect to the relay and either create or join a named room.
// Returns 1 on success, 0 on failure.
int NetLink_Connect(const char *room_name, int as_host);

// Our own assigned player slot once paired: 0 = host/parent, 1-3 = joiner.
// Only valid after NetLink_Connect succeeds.
int NetLink_GetAssignedId(void);

// Send our 20-byte payload to every other player in the room.
void NetLink_Send(const unsigned char *data, size_t len);

// Non-blocking: if a payload arrived from player slot `player_index`
// since the last poll, copy up to `max_len` bytes into out_buf and
// return 1. Otherwise return 0 immediately.
int NetLink_PollSlot(int player_index, unsigned char *out_buf, size_t max_len);

void NetLink_Disconnect(void);

#endif
