package com.hack.core.talkingtom;

import android.app.Activity;
import android.app.Instrumentation;
import android.os.Build;
import android.os.Bundle;
import android.os.PersistableBundle;

import androidx.annotation.RequiresApi;

public class HackerInstrumentation extends Instrumentation {

    private Instrumentation mOrigin = null;

    public static HackerInstrumentation INSTANCE() {
        return DefaultHackerInstru.get();
    }

    private HackerInstrumentation(){}


    private static class DefaultHackerInstru {
        private static HackerInstrumentation sInstancce = new HackerInstrumentation();
        static HackerInstrumentation get() {
            return sInstancce;
        }
    }

    public void setOriginInstru(Instrumentation instru) {
        mOrigin = instru;
    }

    @Override
    public void callActivityOnCreate(Activity activity, Bundle icicle) {
        mOrigin.callActivityOnCreate(activity, icicle);

        TalkingTomHacker.afterCreateActivity(activity);
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    @Override
    public void callActivityOnCreate(Activity activity, Bundle icicle, PersistableBundle persistentState) {
        mOrigin.callActivityOnCreate(activity, icicle, persistentState);

        TalkingTomHacker.afterCreateActivity(activity);
    }

    @Override
    public void callActivityOnDestroy(Activity activity) {
        mOrigin.callActivityOnDestroy(activity);

        TalkingTomHacker.afterCreateActivity(activity);
    }

    @Override
    public void callActivityOnResume(Activity activity) {
        mOrigin.callActivityOnResume(activity);

        TalkingTomHacker.afterResumeActivity(activity);
    }
}
