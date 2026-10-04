#include "netlink.h"
#include <string.h>

static int sAssignedId = -1;

int NetLink_GetAssignedId(void) { return sAssignedId; }

#ifdef __ANDROID__
#include <jni.h>

// Set by Java via NetLinkBridge.nativeSetAssignedId() once the relay
// confirms our room pairing.
JNIEXPORT void JNICALL
Java_com_satr_netlink_NetLinkBridge_nativeSetAssignedId(JNIEnv *env, jobject thiz, jint id)
{
    sAssignedId = id;
}

// TODO: JNIEnv/jobject caching so these can call back into
// NetLinkBridge's send/poll methods — needs a JavaVM* stashed at
// JNI_OnLoad, same pattern as most JNI bridges with persistent state.
int NetLink_Connect(const char *room_name, int as_host) { /* calls into NetLinkBridge.connect(...) */ return 0; }
void NetLink_Send(const unsigned char *data, size_t len) { /* calls into NetLinkBridge.send(...) */ }
int NetLink_PollSlot(int player_index, unsigned char *out_buf, size_t max_len) { /* calls into NetLinkBridge.pollSlot(...) */ return 0; }
void NetLink_Disconnect(void) { /* calls into NetLinkBridge.disconnect() */ }
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/websocket.h>
// Real implementation using the browser's native WebSocket directly —
// a later step, separate design pass per what we flagged earlier.
int NetLink_Connect(const char *room_name, int as_host) { return 0; }
void NetLink_Send(const unsigned char *data, size_t len) { }
int NetLink_PollSlot(int player_index, unsigned char *out_buf, size_t max_len) { return 0; }
void NetLink_Disconnect(void) { }
#endif
