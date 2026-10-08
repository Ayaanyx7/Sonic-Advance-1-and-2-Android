package com.satr.netlink;

import android.util.Base64;
import okhttp3.*;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.TimeUnit;

public class NetLinkBridge {
    private static final String RELAY_URL = "wss://advance-net-play-server.onrender.com";

    public interface Listener {
        void onRoomListReceived(List<RoomInfo> rooms);
        void onConnectError(String message);
    }

    public static class RoomInfo {
        public final String name;
        public final int players;
        public RoomInfo(String name, int players) {
            this.name = name;
            this.players = players;
        }
    }

    private final OkHttpClient client;
    private final Listener listener;
    private WebSocket webSocket;
    private volatile boolean connected = false;

    @SuppressWarnings("unchecked")
    private final ConcurrentLinkedQueue<byte[]>[] slotQueues = new ConcurrentLinkedQueue[4];

    private native void nativeSetAssignedId(int id);
    private native void nativeRegisterInstance();

    public NetLinkBridge(Listener listener) {
        this.listener = listener;
        for (int i = 0; i < 4; i++) slotQueues[i] = new ConcurrentLinkedQueue<>();
        client = new OkHttpClient.Builder()
            .readTimeout(0, TimeUnit.MILLISECONDS)
            .build();
        nativeRegisterInstance();
        openSocketIfNeeded();
    }

    // The socket now needs to be open before we can even request a room
    // list, not just when actually connecting to a room — opened once,
    // lazily, on first use.
    private void openSocketIfNeeded() {
        if (webSocket != null) return;
        Request request = new Request.Builder().url(RELAY_URL).build();
        webSocket = client.newWebSocket(request, new WebSocketListener() {
            @Override
            public void onMessage(WebSocket ws, String text) {
                handleMessage(text);
            }

            @Override
            public void onFailure(WebSocket ws, Throwable t, Response response) {
                connected = false;
                if (listener != null) listener.onConnectError("Connection failed: " + t.getMessage());
            }

            @Override
            public void onClosed(WebSocket ws, int code, String reason) {
                connected = false;
            }
        });
    }

    public void requestRoomList(String gameId) {
        openSocketIfNeeded();
        try {
            JSONObject msg = new JSONObject();
            msg.put("type", "list");
            msg.put("game", gameId);
            webSocket.send(msg.toString());
        } catch (Exception ignored) {}
    }

    public void connect(String roomName, boolean asHost, String gameId) {
        openSocketIfNeeded();
        try {
            JSONObject msg = new JSONObject();
            msg.put("type", asHost ? "host" : "join");
            msg.put("room", roomName);
            msg.put("game", gameId);
            webSocket.send(msg.toString());
        } catch (Exception ignored) {}
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
            } else if (type.equals("roomlist")) {
                JSONArray arr = obj.getJSONArray("rooms");
                List<RoomInfo> rooms = new ArrayList<>();
                for (int i = 0; i < arr.length(); i++) {
                    JSONObject r = arr.getJSONObject(i);
                    rooms.add(new RoomInfo(r.getString("name"), r.getInt("players")));
                }
                if (listener != null) listener.onRoomListReceived(rooms);
            } else if (type.equals("error")) {
                if (listener != null) listener.onConnectError(obj.optString("message", "Unknown error"));
            }
        } catch (Exception e) {
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
    byte[] latest = null;
    byte[] p;
    while ((p = slotQueues[playerIndex].poll()) != null) latest = p;
    return latest;
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
