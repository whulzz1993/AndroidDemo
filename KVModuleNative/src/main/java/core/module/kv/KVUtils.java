package core.module.kv;

import android.content.Context;
import android.os.ConditionVariable;
import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.IntDef;
import androidx.annotation.NonNull;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class KVUtils {
    private static final String TAG = "KVUtils";

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
            STATUS_ERROR,
            STATUS_OK,
    })
    public @interface KvStat {}

    public final static int STATUS_ERROR = -1;
    public final static int STATUS_OK = 0;

    private Context mAppContext;

    private ConditionVariable mReady = new ConditionVariable(false);
    private KVUtils() {}

    private static final KVUtils getInstance() {
        return DefaultKV.sInstance;
    }

    public static void init(@NonNull Context context) {
        getInstance().initKv(context);
    }

    public static String put(String key, String value) {
        String orig = getInstance().getValue(key);
        if (TextUtils.isEmpty(orig)) {
            if (getInstance().addKeyValueImpl(key, value) != STATUS_OK) {
                Log.e(TAG, String.format("put (%s %s) failed", key, value));
            }
            return null;
        } else {
            if (getInstance().addKeyValueImpl(key, value) != STATUS_OK) {
                Log.e(TAG, String.format("replace (%s %s) -> (%s %s) failed", key, orig, key, value));
            }
            return orig;
        }
    }

    public static String getValue(String key) {
        return getInstance().getValueImpl(key);
    }

    private void initKv(@NonNull Context context) {
        mAppContext = context;
        asyncLoadKv();
    }

    @KvStat
    private int addKeyValueImpl(String key, String value) {
        if (TextUtils.isEmpty(key) || TextUtils.isEmpty(value)) {
            return STATUS_ERROR;
        }
        waitReady();
        if (NSKV(key, value) == 0) {
            return STATUS_OK;
        }
        return STATUS_ERROR;
    }

    private String getValueImpl(String key) {
        if (TextUtils.isEmpty(key)) {
            return null;
        }
        waitReady();
        return NGKV(key);
    }

    private void asyncLoadKv() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    ensureKvDir();
                    System.loadLibrary("modulekv");
                    String dataDir = mAppContext.getApplicationInfo().dataDir;
                    tryExtractFromAsset("kv", dataDir, "files/kv/init.kv");
                    NKVI(dataDir, true);
                    mReady.open();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }, "KVinit").start();
    }

    private void ensureKvDir() {
        File file = mAppContext.getFilesDir();
        File kvFile = new File(file, "kv");
        if (kvFile.exists()) {
            return;
        }
        kvFile.mkdirs();
    }

    private void waitReady() {
        mReady.block();
    }

    private static class DefaultKV {
        static final KVUtils sInstance = createDefaultKV();
        private static KVUtils createDefaultKV() {
            return new KVUtils();
        }
    }

    private boolean tryExtractFromAsset(String assetPath, String dstDir, String dstPath) {
        boolean ret = false;
        if (mAppContext != null) {
            InputStream is = null;
            OutputStream os = null;
            try {
                File dstFile = new File(dstDir, dstPath);
                if (!dstFile.exists()) {
                    is = mAppContext.getAssets().open(assetPath);
                    os = new FileOutputStream(dstFile);

                    byte[] buf = new byte[1024];
                    int size = 0;
                    while ((size = is.read(buf)) != -1) {
                        os.write(buf, 0, size);
                    }

                    os.flush();
                }

                if (dstFile.exists() && dstFile.length() > 0) {
                    ret = true;
                }

            } catch (Exception ignore) {

            } finally {
                if (os != null) {
                    try {
                        os.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
                if (is != null) {
                    try {
                        is.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
        return ret;
    }

    /**
     * @param dataDir
     * @param isServer
     * @return
     */
    private native int NKVI(String dataDir, boolean isServer);

    /**
     *
     * @param key
     * @param value
     * @return
     */
    private native int NSKV(String key, String value);

    /**
     *
     * @param key
     * @return
     */
    private native String NGKV(String key);
}
