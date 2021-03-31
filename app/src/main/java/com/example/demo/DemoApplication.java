package com.example.demo;

import android.app.Application;
import android.content.Context;

import core.module.kv.KVUtils;

public class DemoApplication extends Application {

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
    }

    @Override
    public void onCreate() {
        super.onCreate();
        KVUtils.init(this);
    }
}
