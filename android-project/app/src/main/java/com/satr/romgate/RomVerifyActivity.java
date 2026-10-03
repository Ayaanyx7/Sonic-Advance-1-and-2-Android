package com.satr.romgate;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class RomVerifyActivity extends Activity {

    private static final String PREFS_NAME = "rom_verify";
    private static final String PREF_KEY_VERIFIED = "rom_verified";
    private static final int PICK_ROM_REQUEST = 1001;

    // TODO: confirm this matches whatever SDLActivity actually loads
    static {
        System.loadLibrary("main");
    }

    private TextView statusText;

    private native String nativeVerifyRom(byte[] romData);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        SharedPreferences prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        if (prefs.getBoolean(PREF_KEY_VERIFIED, false)) {
            launchGame();
            return;
        }

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setGravity(Gravity.CENTER);
        layout.setPadding(48, 48, 48, 48);

        statusText = new TextView(this);
        statusText.setText("Select your Sonic Advance ROM to continue.");
        statusText.setGravity(Gravity.CENTER);
        statusText.setTextSize(16);
        layout.addView(statusText);

        Button pickButton = new Button(this);
        pickButton.setText("Select ROM");
        pickButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                openRomPicker();
            }
        });
        layout.addView(pickButton);

        setContentView(layout);
    }

    private void openRomPicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent, PICK_ROM_REQUEST);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != PICK_ROM_REQUEST || resultCode != RESULT_OK || data == null) {
            return;
        }

        Uri uri = data.getData();
        if (uri == null) {
            return;
        }

        byte[] romBytes = readAllBytes(uri);
        if (romBytes == null) {
            statusText.setText("Couldn't read that file. Try again.");
            return;
        }

        String matchName = nativeVerifyRom(romBytes);

        if (matchName != null) {
            SharedPreferences prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
            prefs.edit().putBoolean(PREF_KEY_VERIFIED, true).apply();
            launchGame();
        } else {
            statusText.setText("That file doesn't match a known Sonic Advance ROM. Try a different file.");
            Toast.makeText(this, "ROM verification failed", Toast.LENGTH_SHORT).show();
        }
    }

    private byte[] readAllBytes(Uri uri) {
        try {
            InputStream in = getContentResolver().openInputStream(uri);
            if (in == null) return null;

            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buffer = new byte[64 * 1024];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
            in.close();
            return out.toByteArray();
        } catch (IOException e) {
            return null;
        }
    }

    private void launchGame() {
        // Stock org.libsdl.app.SDLActivity, referenced directly — identical across
        // every game branch since it's never been subclassed per-game.
        Intent intent = new Intent(this, org.libsdl.app.SDLActivity.class);
        startActivity(intent);
        finish();
    }
}
