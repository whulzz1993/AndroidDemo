package com.hack.core.talkingtom;

import android.app.Activity;
import android.app.Instrumentation;
import android.graphics.SurfaceTexture;
import android.media.MediaPlayer;
import android.os.ConditionVariable;
import android.text.TextUtils;
import android.util.Log;
import android.view.TextureView;
import android.view.View;

import androidx.annotation.NonNull;

import com.hack.utils.HandlerUtils;
import com.hack.utils.RefUtils;

import java.lang.ref.WeakReference;
import java.lang.reflect.Field;

public class TalkingTomHacker {
    private static final String TAG = "TomHacker";
    private static WeakReference<TextureView.SurfaceTextureListener> sOldListener = null;

    private static ConditionVariable sModifyMediaPlayerCondition = new ConditionVariable(false);

    private static TextureView.SurfaceTextureListener sSurfaceTextureListener = new TextureView.SurfaceTextureListener() {
        @Override
        public void onSurfaceTextureAvailable(@NonNull SurfaceTexture surface, int width, int height) {
            getOldTextureListener().onSurfaceTextureAvailable(surface, width, height);
            sModifyMediaPlayerCondition.open();
        }

        @Override
        public void onSurfaceTextureSizeChanged(@NonNull SurfaceTexture surface, int width, int height) {
            getOldTextureListener().onSurfaceTextureSizeChanged(surface, width, height);
        }

        @Override
        public boolean onSurfaceTextureDestroyed(@NonNull SurfaceTexture surface) {
            return getOldTextureListener().onSurfaceTextureDestroyed(surface);
        }

        @Override
        public void onSurfaceTextureUpdated(@NonNull SurfaceTexture surface) {
            getOldTextureListener().onSurfaceTextureUpdated(surface);
        }
    };
    public static void initInstrumentation() {
        final String CLASS_ACTIVITYTHREAD = "android.app.ActivityThread";

        RefUtils.MethodRef<Object> currentActivityThreadRef =
                new RefUtils.MethodRef<Object>(CLASS_ACTIVITYTHREAD, true,
                        "currentActivityThread", new Class[0]);
        Object activityThread = currentActivityThreadRef.invoke(null, new Object[0]);

        RefUtils.FieldRef<Instrumentation> instrumentationFieldRef =
                new RefUtils.FieldRef<Instrumentation>(CLASS_ACTIVITYTHREAD, false, "mInstrumentation");
        Instrumentation appInstru = instrumentationFieldRef.get(activityThread);

        HackerInstrumentation.INSTANCE().setOriginInstru(appInstru);

        instrumentationFieldRef.set(activityThread, HackerInstrumentation.INSTANCE());
    }

    public static void afterCreateActivity(Activity activity) {
        Log.d(TAG, "afterCreateActivity " + activity);
        try {
            disableRewardActivityMediaPlayer(activity);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void afterResumeActivity(Activity activity) {
        disableRewardActivityResume(activity);
    }

    private static void disableRewardActivityResume(Activity activity) {
        final String REWARD_ACTIVITY_NAME = "com.miui.zeus.mimo.sdk.ad.reward.RewardVideoAdActivity";
        if (!TextUtils.equals(activity.getLocalClassName(), REWARD_ACTIVITY_NAME)) {
            return;
        }

        synchronized (TalkingTomHacker.class) {
            if (activity.isDestroyed() || activity.isFinishing()) {
                return;
            }
            activity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    pretendClickCloseActivity(activity, "mimo_reward_close_img");
                }
            });
        }
    }

    private static void disableRewardActivityMediaPlayer(Activity activity) throws Exception {
        final String REWARD_ACTIVITY_NAME = "com.miui.zeus.mimo.sdk.ad.reward.RewardVideoAdActivity";
        if (!TextUtils.equals(activity.getLocalClassName(), REWARD_ACTIVITY_NAME)) {
            return;
        }

        final String VIDEO_AD_VIEW_FIELD_TYPE = "com.miui.zeus.mimo.sdk.video.VideoAdView";

        Field tmpRef = null;
        Field[] fields = activity.getClass().getDeclaredFields();
        for (Field field : fields) {
            if (TextUtils.equals(field.getType().getName(), VIDEO_AD_VIEW_FIELD_TYPE)) {
                tmpRef = field;
                break;
            }
        }

        if (tmpRef == null) {
            return;
        }

        tmpRef.setAccessible(true);

        Object videoAdView = tmpRef.get(activity);

        if (videoAdView == null) {
            Log.e(TAG, "videoAdView is null");
            return;
        }

        final String TEXTURE_VIDEO_VIEW_TYPE = "com.miui.zeus.mimo.sdk.video.TextureVideoView";

        tmpRef = null;

        fields = videoAdView.getClass().getDeclaredFields();

        for (Field field : fields) {
            if (TextUtils.equals(field.getType().getName(), TEXTURE_VIDEO_VIEW_TYPE)) {
                tmpRef = field;
                break;
            }
        }

        if (tmpRef == null) {
            return;
        }

        tmpRef.setAccessible(true);
        TextureView textureVideoView = (TextureView) tmpRef.get(videoAdView);

        if (textureVideoView == null) {
            Log.e(TAG, "textureVideoView null");
            return;
        }

        final String MEDIA_PLAYER_TYPE = "android.media.MediaPlayer";
        tmpRef = null;
        fields = textureVideoView.getClass().getDeclaredFields();

        for (Field field : fields) {
            if (TextUtils.equals(field.getType().getName(), MEDIA_PLAYER_TYPE)) {
                tmpRef = field;
                break;
            }
        }

        if (tmpRef == null) {
            return;
        }

        TextureView.SurfaceTextureListener originListener = textureVideoView.getSurfaceTextureListener();

        if (originListener == null) {
            Log.e(TAG, "origin surface texure Listener null");
            return;
        }

        setOldTextureListener(originListener);

        textureVideoView.setSurfaceTextureListener(sSurfaceTextureListener);

        final Field mediaPlayerRef = tmpRef;

        HandlerUtils.post(new Runnable() {
            @Override
            public void run() {
                sModifyMediaPlayerCondition.block();
                sModifyMediaPlayerCondition.close();

                mediaPlayerRef.setAccessible(true);
                MediaPlayer player = null;

                long start = System.currentTimeMillis();
                //20ms后再试图访问MediaPlayer
                long maxTime = start + 20;
                try {
                    while (System.currentTimeMillis() < maxTime);
                    player = (MediaPlayer) mediaPlayerRef.get(textureVideoView);
                } catch (IllegalAccessException e) {
                    e.printStackTrace();
                }

                if (player == null) {
                    Log.e(TAG, "MediaPlayer is null");
                    return;
                }

                if (player.isPlaying()) {
                    Log.d(TAG, "MediaPlayer is playing");
                    player.pause();
                } else {
                    Log.d(TAG, "MeidPlayer not playing");
                }

                start = System.currentTimeMillis();

                maxTime = start + 1000;

                while (System.currentTimeMillis() < maxTime && !player.isPlaying());

                Log.d(TAG, "wait player playing cost " + (System.currentTimeMillis() - start) + "ms");


                //等待MediaPlayer播放100ms， fix crash
                start = System.currentTimeMillis();

                maxTime = start + 100;

                while (System.currentTimeMillis() < maxTime);

                try {
                    pretendCompleteMediaPlayer(player);
                } catch (Exception e) {
                    e.printStackTrace();
                }
                player.release();
                start = System.currentTimeMillis();
                //20ms后试图close掉activity
                maxTime = start + 20;
                while (System.currentTimeMillis() < maxTime);
                disableRewardActivityResume(activity);
            }
        });
    }

    private static void pretendCompleteMediaPlayer(MediaPlayer player) throws Exception {
        Class mediaPlayerClass = Class.forName("android.media.MediaPlayer");
        Field ff = mediaPlayerClass.getDeclaredField("mOnCompletionListener");
        ff.setAccessible(true);
        MediaPlayer.OnCompletionListener OnCompletionListener = (MediaPlayer.OnCompletionListener) ff.get(player);

        if (OnCompletionListener == null) {
            Log.e(TAG, "OnCompletionListener null");
            return;
        }

        OnCompletionListener.onCompletion(player);
    }

    private static void pretendClickCloseActivity(Activity activity, String resStr) {
        if (activity instanceof View.OnClickListener) {
            int resId = activity.getApplicationContext().getResources().getIdentifier(resStr, "id", activity.getPackageName());

            if (resId <= 0) {
                Log.e(TAG, "pretendClickCloseActivity failed for res " + resStr + " not found");
                return;
            }

            View closeLikelyView = new View(activity.getApplicationContext());
            closeLikelyView.setId(resId);
            ((View.OnClickListener) activity).onClick(closeLikelyView);
            Log.d(TAG, "pretendClickCloseActivity success");
        } else {
            Log.e(TAG, "pretendClickCloseActivity failed for not OnClickListener instance");
        }
    }

    private static TextureView.SurfaceTextureListener getOldTextureListener() {
        return sOldListener.get();
    }

    private static void setOldTextureListener(TextureView.SurfaceTextureListener listener) {
        sOldListener = new WeakReference<>(listener);
    }
}
