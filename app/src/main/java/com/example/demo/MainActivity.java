package com.example.demo;

import android.os.Bundle;

import com.example.demo.test.KVTest;
import com.example.demo.test.Test;
import com.example.javapipe.JavaPipe;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.snackbar.Snackbar;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

import android.util.Log;
import android.view.View;

import core.module.kv.KVUtils;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";
    private JavaPipe mPipe;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
                Test.main();
                mPipe.pipeWrite();
            }
        });
        mPipe = new JavaPipe();
        testPipe();
    }

    private void writeToPipeIfNeeded() {
        mPipe.pipeWrite();
    }

    private void testPipe() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                while (true) {
                    long start = System.currentTimeMillis();
                    mPipe.pipeRead();
                    Log.d(TAG, String.format("pipeRead success cost %dms",
                            System.currentTimeMillis() - start));
                }
            }
        }).start();
    }
}