package com.core.rbtree.util;

public class JniBridge {
    static {
        System.loadLibrary("rbcore");
    }
    public static native int NIRM(String dataDir, boolean isServer);
    public static native void test();
}
