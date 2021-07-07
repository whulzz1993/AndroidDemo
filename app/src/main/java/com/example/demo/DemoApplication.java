package com.example.demo;

import android.app.Application;
import android.content.Context;

import com.example.demo.test.IoUtils;

import core.module.kv.KVUtils;
import core.module.runtimefake.RuntimeFake;

public class DemoApplication extends Application {

    static {
        //System.loadLibrary("gadget");
        System.loadLibrary("aesmodule");
    }
    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
    }

    @Override
    public void onCreate() {
        super.onCreate();
//        KVUtils.init(this);
        //RuntimeFake.fakeRuntime(getApplicationInfo().targetSdkVersion);
    }
}
