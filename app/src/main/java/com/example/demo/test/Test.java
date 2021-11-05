package com.example.demo.test;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.provider.Settings;
import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;

import core.module.kv.KVUtils;

public class Test {

    public static void main(Context context) {
        //KVUtils.init(context);
        //KVTest.test();
        PrivacyApiTest.test(context);
//        new Thread(new Runnable() {
//            @Override
//            public void run() {
//                NetTest.test();
//            }
//        }).start();
    }
}
