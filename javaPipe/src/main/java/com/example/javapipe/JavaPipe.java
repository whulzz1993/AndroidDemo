package com.example.javapipe;

import android.os.ParcelFileDescriptor;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class JavaPipe {
    private static final String TAG = "JavaPipe";
    InputStream mPipeRead = null;
    OutputStream mPipeWrite = null;
    public JavaPipe() {
        initPipe();
    }
    private void initPipe() {
        try {
            ParcelFileDescriptor[] pipes = ParcelFileDescriptor.createPipe();
            mPipeRead = new ParcelFileDescriptor.AutoCloseInputStream(pipes[0]);
            mPipeWrite = new ParcelFileDescriptor.AutoCloseOutputStream(pipes[1]);
        } catch (IOException e) {
            Log.e(TAG, "initPipe failed!", e);
            mPipeRead = null;
            mPipeWrite = null;
        }
    }

    public void pipeRead() {
        if (mPipeRead != null) {
            int tryCount = 3;
            do {
                try {
                    mPipeRead.read();
                    break;
                } catch (IOException e) {
                    e.printStackTrace();
                    continue;
                }
            } while (--tryCount > 0);
        }
    }

    public void pipeWrite() {
        if (mPipeWrite != null) {
            int tryCount = 3;
            do {
                try {
                    mPipeWrite.write(1);
                    break;
                } catch (IOException e) {
                    e.printStackTrace();
                    continue;
                }
            } while (--tryCount > 0);
        }
    }
}