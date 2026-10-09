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
    private static final String ASSET_REVISION = "r367-android-assets-v1"; // game assets unchanged in r368
    private static final int UP = 1, DOWN = 2, LEFT = 4, RIGHT = 8;
    private static final int ACTION = 16, BACK = 32, PAUSE = 64;
    private static final String[] FILES = {
        "AirXonix-cleanroom.zip", "AirXonix.wrp.exe",
        "MUSIC/00.mus", "MUSIC/01.mus", "MUSIC/02.mus",
        "MUSIC/03.mus", "MUSIC/04.mus", "MUSIC/05.mus",
        "MUSIC/06.mus", "MUSIC/07.mus", "MUSIC/08.mus",
        "MUSIC/09.mus", "MUSIC/29.MUS"
    };

    private static native void nativeSetPadMask(int mask);
    private NativePadView padOverlay;

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
            padOverlay = new NativePadView(this);
            mLayout.addView(padOverlay, new RelativeLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        }
    }

    private void installAssets() throws IOException {
        File root = getFilesDir();
        SharedPreferences preferences = getSharedPreferences("airxonix_assets", MODE_PRIVATE);
        boolean upToDate = ASSET_REVISION.equals(preferences.getString("revision", ""));
        AssetManager assets = getAssets();
        // Public clean-room builds lack commercial EXE/MUSIC. Keep them playable;
        // a personally packed APK still installs the complete original set.
        boolean bundledOriginal = false;
        try (InputStream check = assets.open("AirXonix.wrp.exe")) {
            bundledOriginal = check.read() >= 0;
        } catch (IOException ignored) { /* clean-room-only APK */ }
        int count = bundledOriginal ? FILES.length : 1;
        if (upToDate) {
            for (int i = 0; i < count; i++) {
                File f = new File(root, FILES[i]);
                if (!f.isFile() || f.length() == 0) { upToDate = false; break; }
            }
        }
        if (upToDate) return;
        for (int i = 0; i < count; i++) {
            String name = FILES[i];
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
        // gameinf.bin and hscore.bin are intentionally NOT in FILES: preserve saves.
        if (!preferences.edit().putString("revision", ASSET_REVISION).commit())
            Log.w(TAG, "Could not persist installed asset revision");
        Log.i(TAG, (bundledOriginal ? "Original EXE/MUSIC + clean-room" : "Clean-room")
            + " assets installed in " + root);
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
        if (padOverlay != null) padOverlay.releaseAll();
        if (!mBrokenLibraries) nativeSetPadMask(0);
        super.onPause();
    }

    // One analogue thumbstick drives the game's cardinal movement. Our game
    // does not allow simultaneous diagonal travel, so we select the dominant
    // component after a circular deadzone. Buttons remain multi-touch capable.
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
        private float stickRadius() { return step() * 1.55f; }
        private float thumbTravel() { return step() * .97f; }
        private static float dist(float x,float y,float xx,float yy) {
            return (float)Math.hypot(x-xx,y-yy);
        }
        private int lastMask = 0;
        private int stickPointerId = -1;
        private float stickX = 0f, stickY = 0f;

        private void drawStick(Canvas c) {
            final float r=stickRadius(), x=cx(), y=cy();
            p.setStyle(Paint.Style.FILL);
            p.setColor(Color.argb(75, 20, 33, 54));
            c.drawCircle(x,y,r,p);
            p.setStyle(Paint.Style.STROKE);
            p.setStrokeWidth(Math.max(2f,step()*.03f));
            p.setColor(Color.argb(165,215,227,244));
            c.drawCircle(x,y,r,p);
            p.setColor(Color.argb(70,215,227,244));
            c.drawCircle(x,y,r*.43f,p);
            p.setStyle(Paint.Style.FILL);
            p.setColor(stickPointerId>=0 ? Color.argb(195,112,191,255) : Color.argb(145,115,142,180));
            c.drawCircle(x+stickX*thumbTravel(),y+stickY*thumbTravel(),r*.42f,p);
            p.setStyle(Paint.Style.STROKE);
            p.setStrokeWidth(Math.max(2f,step()*.022f));
            p.setColor(Color.argb(200,240,246,255));
            c.drawCircle(x+stickX*thumbTravel(),y+stickY*thumbTravel(),r*.42f,p);
            p.setStyle(Paint.Style.FILL);
        }

        @Override protected void onDraw(Canvas c) {
            float u=step();
            drawStick(c);
            oval(c,bx(),by(),u*.94f,"A",(lastMask & ACTION)!=0);
            oval(c,bx()-u*2,by()+u*.45f,u*.85f,"B",(lastMask & BACK)!=0);
            oval(c,getWidth()*.90f,getHeight()*.19f,u*.62f,"Ⅱ",(lastMask & PAUSE)!=0);
        }

        private int actionAt(float x,float y) {
            float u=step();
            if (dist(x,y,bx(),by())<u*1.05f) return ACTION;
            if (dist(x,y,bx()-u*2,by()+u*.45f)<u*.95f) return BACK;
            if (dist(x,y,getWidth()*.90f,getHeight()*.19f)<u*.75f) return PAUSE;
            return 0;
        }

        private void setStickPosition(float px,float py) {
            // Clamp displacement to circular thumb travel; no quadratic dead
            // zones, so the joystick starts moving smoothly after 20% travel.
            float dx=(px-cx())/stickRadius(), dy=(py-cy())/stickRadius();
            float len=(float)Math.hypot(dx,dy);
            if(len>1f){dx/=len;dy/=len;}
            stickX=dx;stickY=dy;
        }
        private int stickDirection() {
            return stickPointerId<0 ? 0 : VirtualStickDirection.direction(stickX,stickY);
        }
        private void updateNative(int mask) {
            if(mask!=lastMask){
                lastMask=mask;
                nativeSetPadMask(mask);
            }
            invalidate();
        }
        void releaseAll() {
            stickPointerId=-1;
            stickX=0f;stickY=0f;
            updateNative(0);
        }

        @Override public boolean onTouchEvent(MotionEvent event) {
            int action=event.getActionMasked();
            if(action==MotionEvent.ACTION_CANCEL){releaseAll();return true;}
            int lifted=(action==MotionEvent.ACTION_UP || action==MotionEvent.ACTION_POINTER_UP)
                ? event.getActionIndex() : -1;
            if(lifted>=0 && event.getPointerId(lifted)==stickPointerId){
                stickPointerId=-1;stickX=0f;stickY=0f;
            }
            // Grab the analogue pointer only on DOWN. Fingers pressing A/B
            // cannot steal it, and moving off the base keeps the stick held.
            if(action==MotionEvent.ACTION_DOWN || action==MotionEvent.ACTION_POINTER_DOWN){
                int idx=event.getActionIndex();
                if(stickPointerId<0 && dist(event.getX(idx),event.getY(idx),cx(),cy())
                    <stickRadius()*1.35f){
                    stickPointerId=event.getPointerId(idx);
                }
            }
            int mask=0;
            for(int i=0;i<event.getPointerCount();i++) {
                if(i==lifted)continue;
                if(event.getPointerId(i)==stickPointerId){
                    setStickPosition(event.getX(i),event.getY(i));
                }else{
                    mask|=actionAt(event.getX(i),event.getY(i));
                }
            }
            mask|=stickDirection();
            updateNative(mask);
            return true;
        }
    }
}
