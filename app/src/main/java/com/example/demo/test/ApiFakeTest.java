package com.example.demo.test;

import android.os.Build;
import android.util.Log;

import java.lang.reflect.Method;

public class ApiFakeTest {
    public static void fakeHiddenAPiTest() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P) {
            return;
        }
        System.loadLibrary("apifake");

        try {
            Class runtimeClass = Class.forName("dalvik.system.VMRuntime");
            Method nativeLoadMethod = runtimeClass.getDeclaredMethod("setTargetSdkVersionNative",
                    new Class[] {int.class});

            Log.d("whulzz", "setTargetSdkVersionNative success!");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
