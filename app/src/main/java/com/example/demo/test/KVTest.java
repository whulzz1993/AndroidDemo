package com.example.demo.test;

import android.text.TextUtils;
import android.util.Log;

import core.module.kv.KVUtils;

public class KVTest {

    private static final String TAG = "KVTest";
    private static final String[][] pairs = {
            {"1", "test1"},
            {"3", "test3"},
            {"5", "test5"}
    };
    public static void test() {
        for (int i = 0; i < pairs.length; i++) {
            KVUtils.put(pairs[i][0], pairs[i][1]);
        }

        for (int i = 0; i < pairs.length; i++) {
            if (!TextUtils.equals(KVUtils.getValue(pairs[i][0]), pairs[i][1])) {
                Log.e(TAG, String.format("test failed for %s (%s vs %s)",
                        pairs[i][0], pairs[i][1], KVUtils.getValue(pairs[i][0])));
            } else {
                Log.d(TAG, String.format("test success for %s %s",
                        pairs[i][0], KVUtils.getValue(pairs[i][0])));
            }
        }
    }
}
