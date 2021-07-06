package core.module.runtimefake;

import android.os.Build;

public class RuntimeFake {

    static {
        System.loadLibrary("runtimefake");
    }

    public static void fakeRuntime(int targetSdk) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            fake(targetSdk);
        }
    }
    /**
     *
     */
    private static native void fake(int targetSdk);
}
