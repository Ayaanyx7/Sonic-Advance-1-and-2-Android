package org.sega.sonicadv2;

import android.app.AlertDialog;
import android.os.Bundle;
import android.text.InputType;
import android.view.Gravity;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.Toast;

import com.satr.netlink.NetLinkBridge;
import org.libsdl.app.SDLActivity;

import java.util.List;

public class GameActivity extends SDLActivity implements NetLinkBridge.Listener {

    private NetLinkBridge netLinkBridge;
    private String currentGameId;

    private native void nativeRegisterActivity();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        netLinkBridge = new NetLinkBridge(this);
        nativeRegisterActivity();
    }

    // Called from native code via JNI, on the native/game thread.
    public void showMultiplayerMenu(String gameId) {
        currentGameId = gameId;
        runOnUiThread(this::showHostOrJoinChooser);
    }

    private void showHostOrJoinChooser() {
        new AlertDialog.Builder(this)
            .setTitle("Multiplayer")
            .setItems(new CharSequence[]{"Host a room", "Join a room"}, (dialog, which) -> {
                if (which == 0) showHostDialog();
                else showJoinLoadingThenList();
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    private void showHostDialog() {
        final EditText roomNameInput = new EditText(this);
        roomNameInput.setHint("Room name");
        roomNameInput.setInputType(InputType.TYPE_CLASS_TEXT);

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setGravity(Gravity.CENTER);
        layout.setPadding(48, 24, 48, 0);
        layout.addView(roomNameInput);

        new AlertDialog.Builder(this)
            .setTitle("Host a room")
            .setView(layout)
            .setPositiveButton("Host", (dialog, which) -> {
                String room = roomNameInput.getText().toString().trim();
                if (!room.isEmpty()) {
                    netLinkBridge.connect(room, true, currentGameId);
                }
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    private void showJoinLoadingThenList() {
        Toast.makeText(this, "Looking for rooms...", Toast.LENGTH_SHORT).show();
        netLinkBridge.requestRoomList(currentGameId);
        // The list itself arrives async via onRoomListReceived below.
    }

    @Override
    public void onRoomListReceived(List<NetLinkBridge.RoomInfo> rooms) {
        runOnUiThread(() -> {
            if (rooms.isEmpty()) {
                Toast.makeText(this, "No open rooms found.", Toast.LENGTH_SHORT).show();
                return;
            }

            CharSequence[] labels = new CharSequence[rooms.size()];
            for (int i = 0; i < rooms.size(); i++) {
                NetLinkBridge.RoomInfo r = rooms.get(i);
                labels[i] = r.name + " (" + r.players + "/4)";
            }

            new AlertDialog.Builder(this)
                .setTitle("Join a room")
                .setItems(labels, (dialog, which) -> {
                    NetLinkBridge.RoomInfo chosen = rooms.get(which);
                    netLinkBridge.connect(chosen.name, false, currentGameId);
                })
                .setNegativeButton("Cancel", null)
                .show();
        });
    }

    @Override
    public void onConnectError(String message) {
        runOnUiThread(() -> Toast.makeText(this, message, Toast.LENGTH_LONG).show());
    }
}
