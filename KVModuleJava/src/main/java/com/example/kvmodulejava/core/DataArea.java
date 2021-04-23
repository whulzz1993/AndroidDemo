package com.example.kvmodulejava.core;

import android.util.Log;

public class DataArea {

    private static final String TAG = "DataArea";

    public static final String SEPARATOR = ".";

    int flag;
    String prefix;
    DataNode rootNode;

    protected DataArea(int flag, String prefix) {
        this.flag = flag;
        this.prefix = prefix;
        rootNode = new DataNode("AREA");
    }

    protected DataInfo find(String key) {
        return find(rootNode, key, null, false);
    }

    protected boolean set(String key, String value) {
        DataInfo info = find(rootNode, key, value, false);
        if (info != null) {
            info.key = key;
            info.value = value;
            return true;
        } else {
            return add(key, value);
        }
    }

    protected String get(String key) {
        DataInfo info = find(key);
        return info != null ? info.value : null;
    }

    private boolean add(String key, String value) {
        DataInfo info = find(rootNode, key, value, true);
        if (info != null) {
            Log.d(TAG, String.format("add %s %s success!", key, value));
            return true;
        } else {
            Log.e(TAG, String.format("add %s %s failed!", key, value));
            return false;
        }
    }

    private DataNode newNode(String str) {
        return new DataNode(str);
    }

    private DataInfo newInfo(String key, String value) {
        return new DataInfo(key, value);
    }

    private DataInfo find(DataNode node, String key, String value, boolean allocate) {
        if (node == null) {
            return null;
        }
        String remainingStr = key;
        DataNode current = node;
        while (true) {
            int fristSepIndex = remainingStr.indexOf(SEPARATOR);
            boolean wantSub = fristSepIndex != -1;
            int subStrSize = wantSub ? fristSepIndex : remainingStr.length();
            if (subStrSize == 0) {
                return null;
            }

            final String subStr = wantSub ? remainingStr.substring(0, subStrSize) : remainingStr;

            DataNode root = null;
            DataNode children = current.children;
            if (children != null) {
                root = children;
            } else if (allocate) {
                root = newNode(subStr);
                current.children = root;

            } else if (Utils.isResType(flag)) {
                break;
            }

            if (root == null) {
                return null;
            }

            current = findNode(root, subStr, allocate);
            if (current == null) {
                return null;
            }
            if (!wantSub) {
                break;
            }
            remainingStr = remainingStr.substring(++fristSepIndex);
        }

        if (current.info != null) {
            return current.info;
        } else if (allocate) {
            DataInfo info = newInfo(key, value);
            current.info = info;
            return info;
        } else {
            return null;
        }
    }

    private DataNode findNode(DataNode node, String str, boolean allocate) {
        if (node == null) {
            return null;
        }
        DataNode current = node;
        while (true) {
            int comp = str.compareTo(current.nodeStr);
            if (comp == 0) {
                return current;
            }
            if (comp < 0) {
                if (current.left != null) {
                    current = current.left;
                } else {
                    if (!allocate) {
                        return null;
                    }
                    current.left = newNode(str);
                    return current.left;
                }
            } else {
                if (current.right != null) {
                    current = current.right;
                } else {
                    if (!allocate) {
                        return null;
                    }
                    current.right = newNode(str);
                    return current.right;
                }
            }
        }
    }
}
