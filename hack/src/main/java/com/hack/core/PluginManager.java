package com.hack.core;

import android.content.Context;

import com.hack.core.talkingtom.TalkingTomHacker;

public class PluginManager {
    public static void init(Context context) {
        TalkingTomHacker.initInstrumentation();
    }
}
