package com.hack.utils;

import android.os.Handler;
import android.os.HandlerThread;

public class HandlerUtils {

    private static HandlerThread sThread = new HandlerThread("plugin-HU");

    private static Handler sHandler;

    static {
        sThread.start();
        sHandler = new Handler(sThread.getLooper());
    }

    public static void post(Runnable run) {
        sHandler.post(run);
    }

    public static void postDelay(Runnable run, long delay) {
        sHandler.postDelayed(run, delay);
    }
}
