#include "netlink.h"
#include <string.h>

static int sAssignedId = -1;

int NetLink_GetAssignedId(void) { return sAssignedId; }

const char *NetLink_GetGameId(void)
{
#if GAME == GAME_SA1
    return "sa1";
#elif GAME == GAME_SA2
    return "sa2";
#else
    return "unknown";
#endif
}

#ifdef __ANDROID__
#include <jni.h>

static JavaVM *sJavaVM = NULL;

static jobject sBridgeInstance = NULL;
static jclass sBridgeClass = NULL;
static jmethodID sMidConnect, sMidSend, sMidPollSlot, sMidDisconnect;

static jobject sGameActivityInstance = NULL;
static jclass sGameActivityClass = NULL;
static jmethodID sMidShowMultiplayerMenu;

static volatile int sPendingMultiplayerStart = 0;

static JNIEnv *GetJNIEnv(void)
{
    JNIEnv *env;
    jint res;
    (*sJavaVM)->GetEnv(sJavaVM, (void **)&env, JNI_VERSION_1_6);
    if (res == JNI_EDETACHED) {
        (*sJavaVM)->AttachCurrentThread(sJavaVM, &env, NULL);
    }
    return env;
}

JNIEXPORT void JNICALL
Java_com_satr_netlink_NetLinkBridge_nativeRegisterInstance(JNIEnv *env, jobject thiz)
{
    (*env)->GetJavaVM(env, &sJavaVM);
    sBridgeInstance = (*env)->NewGlobalRef(env, thiz);
    jclass localClass = (*env)->GetObjectClass(env, thiz);
    sBridgeClass = (jclass)(*env)->NewGlobalRef(env, localClass);

    sMidConnect = (*env)->GetMethodID(env, sBridgeClass, "connect", "(Ljava/lang/String;ZLjava/lang/String;)V");
    sMidSend       = (*env)->GetMethodID(env, sBridgeClass, "send", "([B)V");
    sMidPollSlot   = (*env)->GetMethodID(env, sBridgeClass, "pollSlot", "(I)[B");
    sMidDisconnect = (*env)->GetMethodID(env, sBridgeClass, "disconnect", "()V");
}

JNIEXPORT void JNICALL
Java_com_satr_netlink_NetLinkBridge_nativeSetAssignedId(JNIEnv *env, jobject thiz, jint id)
{
    sAssignedId = id;
    sPendingMultiplayerStart = 1;
}

int NetLink_Connect(const char *room_name, int as_host)
{
    if (!sBridgeInstance) return 0;
    JNIEnv *env = GetJNIEnv();
    jstring jroom = (*env)->NewStringUTF(env, room_name);
    jstring jgame = (*env)->NewStringUTF(env, NetLink_GetGameId());
    (*env)->CallVoidMethod(env, sBridgeInstance, sMidConnect, jroom, (jboolean)as_host, jgame);
    (*env)->DeleteLocalRef(env, jroom);
    (*env)->DeleteLocalRef(env, jgame);
    return 1;
}

void NetLink_Send(const unsigned char *data, size_t len)
{
    if (!sBridgeInstance) return;
    JNIEnv *env = GetJNIEnv();
    jbyteArray arr = (*env)->NewByteArray(env, (jsize)len);
    (*env)->SetByteArrayRegion(env, arr, 0, (jsize)len, (const jbyte *)data);
    (*env)->CallVoidMethod(env, sBridgeInstance, sMidSend, arr);
    (*env)->DeleteLocalRef(env, arr);
}

int NetLink_PollSlot(int player_index, unsigned char *out_buf, size_t max_len)
{
    if (!sBridgeInstance) return 0;
    JNIEnv *env = GetJNIEnv();
    jbyteArray arr = (jbyteArray)(*env)->CallObjectMethod(env, sBridgeInstance, sMidPollSlot, player_index);
    if (!arr) return 0;

    jsize len = (*env)->GetArrayLength(env, arr);
    if ((size_t)len > max_len) len = (jsize)max_len;
    (*env)->GetByteArrayRegion(env, arr, 0, len, (jbyte *)out_buf);
    (*env)->DeleteLocalRef(env, arr);
    return 1;
}

void NetLink_Disconnect(void)
{
    if (!sBridgeInstance) return;
    JNIEnv *env = GetJNIEnv();
    (*env)->CallVoidMethod(env, sBridgeInstance, sMidDisconnect);
}

JNIEXPORT void JNICALL
Java_org_sega_sonicadv2_GameActivity_nativeRegisterActivity(JNIEnv *env, jobject thiz)
{
    sGameActivityInstance = (*env)->NewGlobalRef(env, thiz);
    jclass localClass = (*env)->GetObjectClass(env, thiz);
    sGameActivityClass = (jclass)(*env)->NewGlobalRef(env, localClass);
    sMidShowMultiplayerMenu = (*env)->GetMethodID(env, sGameActivityClass, "showMultiplayerMenu", "(Ljava/lang/String;)V");
}

void OpenMultiplayerMenu(void)
{
    if (!sGameActivityInstance) return;
    JNIEnv *env = GetJNIEnv();
    if (!env) return;
    if ((*env)->PushLocalFrame(env, 10) < 0) return;

    jstring jgame = (*env)->NewStringUTF(env, NetLink_GetGameId());
    
    (*env)->CallVoidMethod(env, sGameActivityInstance, sMidShowMultiplayerMenu, jgame);

    (*env)->PopLocalFrame(env, NULL);
}

int NetLink_ConsumePendingMultiplayerStart(void)
{
    if (sPendingMultiplayerStart) {
        sPendingMultiplayerStart = 0;
        return 1;
    }
    return 0;
}
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/websocket.h>
int NetLink_Connect(const char *room_name, int as_host) { return 0; }
void NetLink_Send(const unsigned char *data, size_t len) { }
int NetLink_PollSlot(int player_index, unsigned char *out_buf, size_t max_len) { return 0; }
void NetLink_Disconnect(void) { }
void OpenMultiplayerMenu(void) { }
int NetLink_ConsumePendingMultiplayerStart(void) { return 0; }
#endif
