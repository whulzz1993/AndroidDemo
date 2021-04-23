package com.example.kvmodulejava.core;

public class DataInfo {
    public volatile String key;
    public volatile String value;

    public DataInfo(String key, String value) {
        this.key = key;
        this.value = value;
    }
}
