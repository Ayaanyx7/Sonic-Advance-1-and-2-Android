package com.satr.netlink;

import android.util.Base64;
import okhttp3.*;
import org.json.JSONObject;

import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.TimeUnit;

public class NetLinkBridge {
    private static final String RELAY_URL = "wss://advance-net-play-server.onrender.com";

    private final OkHttpClient client;
    private WebSocket webSocket;
    private volatile boolean connected = false;

    @SuppressWarnings("unchecked")
    private final ConcurrentLinkedQueue<byte[]>[] slotQueues = new ConcurrentLinkedQueue[4];

    private native void nativeSetAssignedId(int id);
    private native void nativeRegisterInstance();

    public NetLinkBridge() {
        for (int i = 0; i < 4; i++) slotQueues[i] = new ConcurrentLinkedQueue<>();
        client = new OkHttpClient.Builder()
            .readTimeout(0, TimeUnit.MILLISECONDS) // WebSockets are long-lived; no read timeout
            .build();
        nativeRegisterInstance();
    }

    public void connect(String roomName, boolean asHost) {
        Request request = new Request.Builder().url(RELAY_URL).build();
        webSocket = client.newWebSocket(request, new WebSocketListener() {
            @Override
            public void onOpen(WebSocket ws, Response response) {
                try {
                    JSONObject msg = new JSONObject();
                    msg.put("type", asHost ? "host" : "join");
                    msg.put("room", roomName);
                    ws.send(msg.toString());
                } catch (Exception ignored) {}
            }

            @Override
            public void onMessage(WebSocket ws, String text) {
                handleMessage(text);
            }

            @Override
            public void onFailure(WebSocket ws, Throwable t, Response response) {
                connected = false;
            }

            @Override
            public void onClosed(WebSocket ws, int code, String reason) {
                connected = false;
            }
        });
    }

    private void handleMessage(String text) {
        try {
            JSONObject obj = new JSONObject(text);
            String type = obj.getString("type");

            if (type.equals("assigned")) {
                connected = true;
                nativeSetAssignedId(obj.getInt("id"));
            } else if (type.equals("data")) {
                int from = obj.getInt("from");
                byte[] bytes = Base64.decode(obj.getString("payload"), Base64.NO_WRAP);
                if (from >= 0 && from < 4) {
                    slotQueues[from].add(bytes);
                }
            }
            // "error" type: nothing reads this yet — worth wiring to the UI
            // once the host/join screen exists, so a full room or bad name
            // shows the player something instead of silently hanging.
        } catch (Exception e) {
            // Malformed message — drop it, never crash the connection over it.
        }
    }

    public void send(byte[] data) {
        if (webSocket == null) return;
        try {
            JSONObject msg = new JSONObject();
            msg.put("type", "data");
            msg.put("payload", Base64.encodeToString(data, Base64.NO_WRAP));
            webSocket.send(msg.toString());
        } catch (Exception ignored) {}
    }

    public byte[] pollSlot(int playerIndex) {
        if (playerIndex < 0 || playerIndex >= 4) return null;
        return slotQueues[playerIndex].poll();
    }

    public void disconnect() {
        if (webSocket != null) {
            webSocket.close(1000, "done");
            webSocket = null;
        }
        connected = false;
    }

    public boolean isConnected() {
        return connected;
    }
            }
