package com.example.kvmodulejava.core;

public class DataNode {
    public volatile DataNode left;
    public volatile DataNode right;
    public volatile DataNode children;
    public volatile DataInfo info;

    String nodeStr;

    public DataNode(String str) {
        this.nodeStr = str;
    }
}
