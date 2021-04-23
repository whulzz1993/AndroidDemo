package com.example.kvmodulejava.core;

import android.util.SparseArray;

public class DataOper {
    SparseArray<DataArea> mAreas = new SparseArray(2);
    public DataOper() {
        initArea();
    }

    private void initArea() {
        mAreas.put(9, new DataArea(9, "*"));
        mAreas.put(10, new DataArea(10, "*"));
    }

    private DataArea getArea(int type, boolean allocate) {
        synchronized (mAreas) {
            DataArea area = mAreas.get(type);
            if (area == null && allocate) {
                area = new DataArea(type, "*");
                mAreas.put(type, area);
            }
            return area;
        }
    }

    public boolean set(int type, String key, String value) {
        DataArea area = getArea(type, true);
        if (area == null) {
            //暂时不支持其他的操作
            return false;
        } else {
            return area.set(key, value);
        }
    }

    public String get(int flag, String key) {
        DataArea area = getArea(flag, false);
        if (area == null) {
            return null;
        } else {
            return area.get(key);
        }
    }
}