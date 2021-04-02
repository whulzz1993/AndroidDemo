package com.example.demo;

import android.Manifest;
import android.os.Build;
import android.os.Bundle;

import com.example.demo.test.MeizuHook;
import com.example.demo.test.RefUtils;
import com.example.demo.test.Test;
import com.example.javapipe.JavaPipe;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.google.android.material.snackbar.Snackbar;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;

import android.os.IBinder;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import java.io.File;
import java.lang.reflect.Method;

public class MainActivity extends AppCompatActivity {


    private static final int REQUEST_MUL_PERMISSIONS = 1000;
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
                Test.main();
                mPipe.pipeWrite();
            }
        });

        Button btn = findViewById(R.id.button);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                MeizuHook.installHook();
                String className = "android.permission.IPermissionManager$Stub";
                RefUtils.MethodRef<IBinder> getServiceMethod =
                        new RefUtils.MethodRef<IBinder>("android.os.ServiceManager",
                                true, "getService", new Class[]{String.class});
                RefUtils.MethodRef<Object> asInterfaceMethod =
                        new RefUtils.MethodRef<Object>(className, true,
                                "asInterface", new Class[]{IBinder.class});
                Object defBinder = getServiceMethod.invoke(null, new Object[]{"permissionmgr"});
                Object binderProxy = asInterfaceMethod.invoke(null, new Object[]{defBinder});

                try {
                    Method isAutoRevokeWhitelisted = binderProxy.getClass().
                            getDeclaredMethod("isAutoRevokeWhitelisted", new Class[] {String.class, int.class});
                    Object ret = isAutoRevokeWhitelisted.invoke(binderProxy, new Object[] {getPackageName(), 0});
                    Log.d(TAG, "");
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        });
        mPipe = new JavaPipe();
        testPipe();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
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

    @RequiresApi(api = Build.VERSION_CODES.M)
    private void requestPermissions() {
        String[] permissions = new String[] {
                Manifest.permission.READ_EXTERNAL_STORAGE,
                Manifest.permission.WRITE_EXTERNAL_STORAGE,
                Manifest.permission.ACCESS_MEDIA_LOCATION,
                "flyme.permission.READ_STORAGE",
        };
        if (permissions.length > 0) {
            Log.d(TAG, "requestPermissions begin");
            requestPermissions(permissions, REQUEST_MUL_PERMISSIONS);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        Log.d(TAG, "requestPermissions end");
    }
}