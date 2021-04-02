package com.example.demo.test;

import android.content.Context;
import android.os.Build;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.text.TextUtils;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

public class IoUtils {
    public static final String EXTERNAL_ANDROID_DATA_DIR = "/Android/data/";
    private static HashSet<String> All_INTERNAL_ROOT_DIR = new HashSet<>();
    private static HashSet<String> ALL_SDCARD_ROOT_DIR = new HashSet<>();
    private static HashSet<String> ALL_EXTERNAL_PRI_DIR = new HashSet<>();
    private static String PKGNAME = null;
    private static int USER_ID = 0;//MU_ENABLE?
    private static String PREF_SDCARD_ROOT = null;
    private static String PREF_DATA_ROOT = null;
    private static String PREF_EXT_PRI_ROOT = null;

    private static boolean INITED = false;
    private static RefUtils.MethodRef<Integer> myUserId =
            new RefUtils.MethodRef<Integer>(
                    "android.os.UserHandle", true,
                    "myUserId", new Class[]{});

    public static void initCache(Context context) {
        synchronized (IoUtils.class) {
            if (INITED) {
                return;
            }
            //trigger system create /sdcard/Android/data/PKG/
            context.getExternalCacheDir();
            getUserSpaceId();
            getAllDataDir(context);
            getAllExternalStorageDir(context);
            getAllExternalPriDir(context);
            initPrefRoot(context);
            PKGNAME = context.getPackageName();
            INITED = true;
        }
    }

    public static int getUserSpaceId() {
        if (!INITED && USER_ID == 0) {
            Integer ret = myUserId.invoke(null, new Object[]{});
            if (ret != null) {
                USER_ID = ret;
            }
        }
        return USER_ID;
    }

    public static HashSet<String> getAllDataDir(Context context) {
        if (!INITED) {
            synchronized (All_INTERNAL_ROOT_DIR) {
                if (All_INTERNAL_ROOT_DIR.isEmpty() && context != null) {
                    String dataDir = context.getApplicationInfo().dataDir;
                    String dataDir1 = getCanonicalPath(dataDir);
                    All_INTERNAL_ROOT_DIR.add(dataDir);
                    All_INTERNAL_ROOT_DIR.add(dataDir1);
                    if (dataDir1 != null && !dataDir1.startsWith("/data/data")) {
                        String dataDir2 = "/data/data/" + context.getPackageName();
                        String dataDir3 = getCurrentUserDataPath(dataDir1);
                        All_INTERNAL_ROOT_DIR.add(dataDir2);
                        All_INTERNAL_ROOT_DIR.add(dataDir3);
                    }
                }
            }
        }
        return All_INTERNAL_ROOT_DIR;
    }

    public static HashSet<String> getAllExternalStorageDir(Context context) {
        if (!INITED) {
            synchronized (ALL_SDCARD_ROOT_DIR) {
                if (ALL_SDCARD_ROOT_DIR.isEmpty() && context != null) {
                    if (new File("/sdcard").exists()) {
                        ALL_SDCARD_ROOT_DIR.add("/sdcard");
                    }
                    ALL_SDCARD_ROOT_DIR.addAll(getExternalStorageList(context));
                    final HashSet<String> canonicalPaths = new HashSet<String>();
                    for (String tmp : ALL_SDCARD_ROOT_DIR) {
                        String canonicalPath = getCanonicalPath(tmp);
                        if (canonicalPath == null) {
                            continue;
                        }
                        canonicalPaths.add(canonicalPath);
                    }
                    ALL_SDCARD_ROOT_DIR.addAll(canonicalPaths);
                }
            }
        }
        return ALL_SDCARD_ROOT_DIR;
    }

    public static HashSet<String> getAllExternalPriDir(Context context) {
        if (!INITED) {
            synchronized (ALL_EXTERNAL_PRI_DIR) {
                for (String externalRoot : ALL_SDCARD_ROOT_DIR) {
                    ALL_EXTERNAL_PRI_DIR.add(externalRoot +
                            EXTERNAL_ANDROID_DATA_DIR + context.getPackageName());
                }
            }
        }
        return ALL_EXTERNAL_PRI_DIR;
    }

    public static String getCanonicalPath(String path) {
        if (TextUtils.isEmpty(path)) {
            return path;
        }
        try {
            return new File(path).getCanonicalPath();
        } catch (IOException ignore) {}

        return path;
    }

    public static String getCurrentUserDataPath(String path) {
        return path.startsWith("/data/data/") ? "/data/user/" + USER_ID + path.substring("/data/data".length()) : path;
    }

    public static String getSubpathFromInternal(String path) {
        for (String dir : All_INTERNAL_ROOT_DIR) {
            if (path.startsWith(dir)) {
                return path.substring(dir.length());
            }
        }
        return null;
    }

    private static void initPrefRoot(Context context) {
        PREF_DATA_ROOT = context.getApplicationInfo().dataDir;
        File externalCacheDir = context.getExternalCacheDir();
        if (externalCacheDir != null) {
            PREF_EXT_PRI_ROOT = externalCacheDir.getParent();
            if (PREF_EXT_PRI_ROOT != null) {
                boolean checkSuccess = false;
                HashSet<String> allSdcards = getAllExternalStorageDir(context);
                for (String sdcard : allSdcards) {
                    if (PREF_EXT_PRI_ROOT.startsWith(sdcard)) {
                        checkSuccess = true;
                        PREF_SDCARD_ROOT = sdcard;
                        break;
                    }
                }
                if (!checkSuccess) {
                    PREF_EXT_PRI_ROOT = null;
                }
            }
        }
    }

    public static String getPreferSdcard() {
        if (PREF_SDCARD_ROOT != null) {
            return PREF_SDCARD_ROOT;
        }
        String prefPath = "/sdcard";
        if (ALL_SDCARD_ROOT_DIR.isEmpty() || ALL_SDCARD_ROOT_DIR.contains(prefPath)) {
            PREF_SDCARD_ROOT = prefPath;
            return PREF_SDCARD_ROOT;
        }

        String randomPath = null;
        for (String sdcard : ALL_SDCARD_ROOT_DIR) {
            if (sdcard.startsWith(prefPath)) {
                PREF_SDCARD_ROOT = sdcard;
                return PREF_SDCARD_ROOT;
            }
            randomPath = sdcard;
        }
        PREF_SDCARD_ROOT = randomPath;
        return PREF_SDCARD_ROOT;
    }

    public static String getPreferExtPriDir() {
        if (PREF_EXT_PRI_ROOT != null) {
            return PREF_EXT_PRI_ROOT;
        }
        if (!TextUtils.isEmpty(PKGNAME)) {
            PREF_EXT_PRI_ROOT = getPreferSdcard() +
                    EXTERNAL_ANDROID_DATA_DIR + PKGNAME;
        }
        return PREF_EXT_PRI_ROOT;
    }

    public static String getPreferDataDir() {
        if (PREF_DATA_ROOT != null) {
            return PREF_DATA_ROOT;
        }
        if (!All_INTERNAL_ROOT_DIR.isEmpty()) {
            String prefPrefix = "/data";
            for (String dataDir : All_INTERNAL_ROOT_DIR) {
                if (dataDir.startsWith(prefPrefix)) {
                    PREF_DATA_ROOT = dataDir;
                    return PREF_DATA_ROOT;
                }
            }
        } else if (!TextUtils.isEmpty(PKGNAME)) {
            PREF_DATA_ROOT = "/data/data/" + PKGNAME;
            return PREF_DATA_ROOT;
        }
        return PREF_DATA_ROOT;
    }

    public static String getSubpathFromSdcard(String path) {
        for (String dir : ALL_SDCARD_ROOT_DIR) {
            if (path.startsWith(dir)) {
                return path.substring(dir.length());
            }
        }
        return null;
    }

    private static List<String> getExternalStorageList(Context context) {
        final HashSet<String> storages = new HashSet<String>();
        final File curExternalStorageDir = Environment.getExternalStorageDirectory();
        if (curExternalStorageDir != null && curExternalStorageDir.exists()) {
            storages.add(curExternalStorageDir.getAbsolutePath());
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1) {
            try {
                final StorageManager storageManager = (StorageManager) context.getSystemService(Context.STORAGE_SERVICE);
                Method getVolumePaths = StorageManager.class.getDeclaredMethod("getVolumePaths");
                getVolumePaths.setAccessible(true);
                String[] paths = (String[]) getVolumePaths.invoke(storageManager);
                if (paths != null && paths.length > 0) {
                    for (String path : paths) {
                        storages.add(path);
                    }
                }
            } catch (Exception e) {
            }
        } else {
        }

        return new ArrayList<String>(storages);
    }
}
