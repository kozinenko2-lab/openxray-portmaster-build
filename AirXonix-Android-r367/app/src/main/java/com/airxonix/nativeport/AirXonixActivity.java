package com.airxonix.nativeport;

import android.app.AlertDialog;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.AssetManager;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Build;
import android.os.Bundle;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RelativeLayout;
import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * Activity uses SDL2's official JNI entrypoint. The original .exe is a DATA
 * CONTAINER only; Android runs libmain.so compiled from native r367 C++.
 */
public final class AirXonixActivity extends SDLActivity {
    private static final String TAG = "AirXonixAndroid";
    private static final String ASSET_REVISION = "r367-android-assets-v2";
    private static final int UP = 1, DOWN = 2, LEFT = 4, RIGHT = 8;
    private static final int ACTION = 16, BACK = 32, PAUSE = 64;
    private static final String[] REQUIRED_FILES = {"AirXonix-cleanroom.zip"};
    private static final String[] OPTIONAL_FILES = {
        "AirXonix.wrp.exe",
        "MUSIC/00.mus", "MUSIC/01.mus", "MUSIC/02.mus",
        "MUSIC/03.mus", "MUSIC/04.mus", "MUSIC/05.mus",
        "MUSIC/06.mus", "MUSIC/07.mus", "MUSIC/08.mus",
        "MUSIC/09.mus", "MUSIC/29.MUS"
    };

    private static native void nativeSetPadMask(int mask);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // SDL thread starts on resume; extract while the Activity is initializing.
        String installFailure = null;
        try {
            installAssets();
        } catch (IOException e) {
            installFailure = e.toString();
            Log.e(TAG, "Bundled resources could not be installed", e);
        }
        super.onCreate(savedInstanceState);
        if (installFailure != null) {
            new AlertDialog.Builder(this)
                .setTitle("AirXonix resources")
                .setMessage(installFailure)
                .setCancelable(false)
                .setPositiveButton("Exit", (dialog, which) -> finish())
                .show();
            return;
        }
        // SDLActivity.mLayout is SDL's RelativeLayout containing the GL surface.
        // Overlay input is driven by JNI, independent of mouse/touch SDL gestures.
        if (mLayout != null) {
            NativePadView overlay = new NativePadView(this);
            mLayout.addView(overlay, new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        }
    }

    private void installAssets() throws IOException {
        File root = getFilesDir();
        SharedPreferences preferences = getSharedPreferences("airxonix_assets", MODE_PRIVATE);
        boolean upToDate = ASSET_REVISION.equals(preferences.getString("revision", ""));
        if (upToDate) {
            for (String name : REQUIRED_FILES) {
                File f = new File(root, name);
                if (!f.isFile() || f.length() == 0) { upToDate = false; break; }
            }
        }
        if (upToDate) return;
        AssetManager assets = getAssets();
        for (String name : REQUIRED_FILES) {
            installOne(assets, root, name);
        }
        for (String name : OPTIONAL_FILES) {
            try (InputStream ignored = assets.open(name)) {
                installOne(assets, root, name);
            } catch (java.io.FileNotFoundException absent) {
                // Public CI builds use only original-free generated graphics/audio.
            }
        }
        // Save data belongs to the application, never APK assets.
        if (!preferences.edit().putString("revision", ASSET_REVISION).commit())
            Log.w(TAG, "Could not persist installed asset revision");
        Log.i(TAG, "Bundled assets installed in " + root);
    }

    private void installOne(AssetManager assets, File root, String name) throws IOException {
            File target = new File(root, name);
            File dir = target.getParentFile();
            if (dir == null || (!dir.isDirectory() && !dir.mkdirs()))
                throw new IOException("Cannot create " + dir);
            File temp = new File(dir, target.getName() + ".partial");
            try (InputStream source = assets.open(name);
                 FileOutputStream dest = new FileOutputStream(temp)) {
                byte[] bytes = new byte[64 * 1024];
                for (int n; (n = source.read(bytes)) != -1; ) dest.write(bytes, 0, n);
                dest.getFD().sync();
            } catch (IOException e) {
                temp.delete();
                throw e;
            }
            if (target.exists() && !target.delete()) throw new IOException("Cannot replace " + target);
            if (!temp.renameTo(target)) throw new IOException("Cannot finish " + target);
        }

    /** Called by SDL game's C++ InputSystem on losing a life. */
    public void vibrateOnDeath(int amplitude, int durationMs) {
        if (durationMs <= 0) return;
        Vibrator vibrator;
        if (Build.VERSION.SDK_INT >= 31) {
            VibratorManager vm = (VibratorManager) getSystemService(Context.VIBRATOR_MANAGER_SERVICE);
            vibrator = vm != null ? vm.getDefaultVibrator() : null;
        } else {
            vibrator = (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);
        }
        if (vibrator != null && vibrator.hasVibrator()) {
            int a = Math.max(1, Math.min(255, amplitude));
            if (!vibrator.hasAmplitudeControl()) a = VibrationEffect.DEFAULT_AMPLITUDE;
            vibrator.vibrate(VibrationEffect.createOneShot(durationMs, a));
        }
    }

    @Override
    protected void onPause() {
        if (!mBrokenLibraries) nativeSetPadMask(0);
        super.onPause();
    }

    private static final class NativePadView extends View {
        private final Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
        NativePadView(Context context) { super(context); setWillNotDraw(false); }

        private void oval(Canvas c, float x, float y, float radius, String label, boolean down) {
            p.setStyle(Paint.Style.FILL);
            p.setColor(down ? Color.argb(157, 140, 208, 255) : Color.argb(85, 26, 38, 58));
            c.drawCircle(x, y, radius, p);
            p.setStyle(Paint.Style.STROKE); p.setStrokeWidth(3);
            p.setColor(Color.argb(150, 234, 239, 255));
            c.drawCircle(x, y, radius, p);
            p.setStyle(Paint.Style.FILL); p.setColor(Color.WHITE);
            p.setTextAlign(Paint.Align.CENTER); p.setTextSize(radius * .78f);
            c.drawText(label, x, y + radius * .28f, p);
        }

        private float step() { return Math.min(getHeight() * .115f, getWidth() * .072f); }
        private float cx() { return getWidth() * .16f; }
        private float cy() { return getHeight() * .70f; }
        private float bx() { return getWidth() * .85f; }
        private float by() { return getHeight() * .69f; }

        @Override protected void onDraw(Canvas c) {
            float u=step();
            oval(c,cx(),cy()-u,u*.78f,"▲",(lastMask & UP)!=0);
            oval(c,cx(),cy()+u,u*.78f,"▼",(lastMask & DOWN)!=0);
            oval(c,cx()-u,cy(),u*.78f,"◀",(lastMask & LEFT)!=0);
            oval(c,cx()+u,cy(),u*.78f,"▶",(lastMask & RIGHT)!=0);
            oval(c,bx(),by(),u*.94f,"A",(lastMask & ACTION)!=0);
            oval(c,bx()-u*2,by()+u*.45f,u*.85f,"B",(lastMask & BACK)!=0);
            oval(c,getWidth()*.90f,getHeight()*.19f,u*.62f,"Ⅱ",(lastMask & PAUSE)!=0);
        }

        private int controlAt(float x, float y) {
            float u=step(), threshold=u*.84f;
            if (dist(x,y,cx(),cy()-u)<threshold) return UP;
            if (dist(x,y,cx(),cy()+u)<threshold) return DOWN;
            if (dist(x,y,cx()-u,cy())<threshold) return LEFT;
            if (dist(x,y,cx()+u,cy())<threshold) return RIGHT;
            if (dist(x,y,bx(),by())<u*1.05f) return ACTION;
            if (dist(x,y,bx()-u*2,by()+u*.45f)<u*.95f) return BACK;
            if (dist(x,y,getWidth()*.90f,getHeight()*.19f)<u*.75f) return PAUSE;
            return 0;
        }
        private float dist(float x,float y,float xx,float yy) {
            return (float)Math.hypot(x-xx,y-yy);
        }

        private int lastMask = 0;
        @Override public boolean onTouchEvent(MotionEvent event) {
            int act=event.getActionMasked();
            int mask=0;
            if (act!=MotionEvent.ACTION_CANCEL) {
                int lifted=act==MotionEvent.ACTION_UP || act==MotionEvent.ACTION_POINTER_UP
                    ? event.getActionIndex():-1;
                for (int i=0;i<event.getPointerCount();i++) {
                    if (i!=lifted) mask |= controlAt(event.getX(i),event.getY(i));
                }
            }
            if (mask!=lastMask) {
                lastMask=mask;
                nativeSetPadMask(mask);
                invalidate();
            }
            return true;
        }
    }
}
