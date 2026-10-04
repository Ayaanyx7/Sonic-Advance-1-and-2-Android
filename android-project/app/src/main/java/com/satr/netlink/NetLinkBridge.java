package com.satr.netlink;

public class NetLinkBridge {
    private native void nativeSetAssignedId(int id);
    // connect(), send(), pollSlot(), disconnect() — actual OkHttp
    // WebSocket methods go here, next step once netlink.c's JNI
    // calls are fleshed out.
}
