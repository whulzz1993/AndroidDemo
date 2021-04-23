package com.example.kvmodulejava.core;

public class KVManager {

    private DataOper mOper;

    private KVManager() {
        mOper = new DataOper();
    }

    public static KVManager getInstance() {
        return defaultManager.sInstance;
    }

    public boolean set(int type, String key, String value) {
        return mOper.set(type, key, value);
    }

    public String get(int type, String key) {
        return mOper.get(type, key);
    }

    private static class defaultManager {
        static final KVManager sInstance = getDefaultInstance();
        static KVManager getDefaultInstance() {
            return new KVManager();
        }
    }
}
