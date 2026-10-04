package org.sega.sonicadv2;

import android.app.AlertDialog;
import android.os.Bundle;
import android.text.InputType;
import android.view.Gravity;
import android.widget.EditText;
import android.widget.LinearLayout;

import com.satr.netlink.NetLinkBridge;
import org.libsdl.app.SDLActivity;

public class GameActivity extends SDLActivity {

    private NetLinkBridge netLinkBridge;

    private native void nativeRegisterActivity();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        netLinkBridge = new NetLinkBridge();
        nativeRegisterActivity();
    }

    // Called from native code via JNI, on the native/game thread —
    // must hop to the UI thread before touching any Android views.
    public void showMultiplayerMenu() {
        runOnUiThread(this::showMultiplayerMenuOnUiThread);
    }

    private void showMultiplayerMenuOnUiThread() {
        final EditText roomNameInput = new EditText(this);
        roomNameInput.setHint("Room name");
        roomNameInput.setInputType(InputType.TYPE_CLASS_TEXT);

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setGravity(Gravity.CENTER);
        layout.setPadding(48, 24, 48, 0);
        layout.addView(roomNameInput);

        new AlertDialog.Builder(this)
            .setTitle("Multiplayer")
            .setView(layout)
            .setPositiveButton("Host", (dialog, which) -> {
                String room = roomNameInput.getText().toString().trim();
                if (!room.isEmpty()) {
                    netLinkBridge.connect(room, true);
                }
            })
            .setNeutralButton("Join", (dialog, which) -> {
                String room = roomNameInput.getText().toString().trim();
                if (!room.isEmpty()) {
                    netLinkBridge.connect(room, false);
                }
            })
            .setNegativeButton("Cancel", null)
            .show();
    }
}
