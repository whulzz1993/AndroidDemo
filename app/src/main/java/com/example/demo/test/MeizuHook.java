package com.example.demo.test;

import android.content.Context;
import android.os.IBinder;
import android.text.TextUtils;
import android.util.Log;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;

public class MeizuHook {

    private static final String TAG = "MeizuHook";
    private static RefUtils.MethodRef<IBinder> getServiceMethod = new RefUtils.MethodRef<IBinder>("android.os.ServiceManager", true, "getService", new Class[]{String.class});
    private static RefUtils.FieldRef<Map<String, Object>> sServiceCacheField =
                new RefUtils.FieldRef<Map<String, Object>>(
                "android.os.ServiceManager", true, "sCache");


    public static void installHook() {
        installForwardingOsProxy();

        new FlymePermissionProxy();
        new AppopsManagerProxy();

        new PermissionManagerProxy();
        new PackageManagerProxy();
    }

    private static void installForwardingOsProxy() {

        Class OsClass = null;
        try {
            OsClass = Class.forName("libcore.io.Os");
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (OsClass == null) {
            Log.e(TAG, "installOs failed for Os not found!");
            return;
        }
        RefUtils.MethodRef<Object> LbcoreOsGetDef = new RefUtils.MethodRef<Object>(OsClass,
                true, "getDefault", new Class[0]);
        Object defObj = null;
        Object newObj = null;
        try {
            defObj = LbcoreOsGetDef.invoke(null, new Object[0]);
            List<Class<?>> interfaces = getAllInterfaces(Class.forName("libcore.io.Linux"));
            if (interfaces != null && interfaces.size() > 0) {
                Class[] ifs = interfaces.toArray(new Class[interfaces.size()]);
                newObj = Proxy.newProxyInstance(defObj.getClass().getClassLoader(), ifs, new ForwardingOsProxy(defObj));
            }
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }

        RefUtils.MethodRef<Boolean> compareAndSetDefaultMethod =
                new RefUtils.MethodRef<Boolean>("libcore.io.Os",
                true, "compareAndSetDefault",
                        new Class[]{OsClass, OsClass});
        do {
            Log.d(TAG, "installOs try...");
        } while (!compareAndSetDefaultMethod.invoke(null, new Object[]{defObj, newObj}));
        Log.d(TAG, "installOs success!");
    }

    private static class BinderHook implements InvocationHandler {
        protected Object mOrigin;
        private String mClassName;
        private String mServiceName;
        BinderHook(final String className, final String serviceName) {
            mClassName = className;
            mServiceName = serviceName;
            installBinderHook(className, serviceName);
        }

        public InvocationHandler createHandle(Object originService) {
            mOrigin = originService;
            return this;
        }

        protected void afterInstallBinder(Object origin, Object newObj) {

        }

        private void installBinderHook(final String className, final String serviceName) {
            Object defBinder = sServiceCacheField.get(null).get(serviceName);
            if (defBinder == null) {
                defBinder = getServiceMethod.invoke(null, new Object[]{serviceName});
            }
            if (defBinder == null) {
                Log.e(TAG, "installBinderHook failed for " + className + " " + serviceName);
                return;
            }

            Class stubClass = null;
            try {
                stubClass = Class.forName(className);
            } catch (ClassNotFoundException e) {
                e.printStackTrace();
            }

            if (stubClass == null) {
                Log.e(TAG, "installBinderHook failed for stubClass==null");
                return;
            }
            RefUtils.MethodRef<Object> asInterfaceMethod =
                    new RefUtils.MethodRef<Object>(stubClass, true, "asInterface", new Class[]{IBinder.class});

            Object origin = asInterfaceMethod.invoke(null, new Object[]{defBinder});
            Object newObj = null;

            List<Class<?>> interfaces = getAllInterfaces(origin.getClass());

            if (interfaces != null && interfaces.size() > 0) {
                Class[] ifs = interfaces.toArray(new Class[interfaces.size()]);
                newObj = Proxy.newProxyInstance(origin.getClass().getClassLoader(),
                        ifs, createHandle(origin));
                new ServiceManagerHook(defBinder,origin, newObj).install(serviceName);
                afterInstallBinder(origin, newObj);
            }
        }

        @Override
        public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
            Log.d(TAG, String.format("invoke %s %s args: %s",
                    mServiceName, method.getName(), Arrays.toString(args)));
            return method.invoke(mOrigin, args);
        }
    }

//    private static void installBinderHook(final String className, final String serviceName) {
//        Object defBinder = sServiceCacheField.get(null).get(serviceName);
//        if (defBinder == null) {
//            defBinder = getServiceMethod.invoke(null, new Object[]{serviceName});
//        }
//        if (defBinder == null) {
//            Log.e(TAG, "installBinderHook failed for " + className + " " + serviceName);
//            return;
//        }
//
//        Class stubClass = null;
//        try {
//            stubClass = Class.forName(className);
//        } catch (ClassNotFoundException e) {
//            e.printStackTrace();
//        }
//
//        if (stubClass == null) {
//            Log.e(TAG, "installBinderHook failed for stubClass==null");
//            return;
//        }
//        RefUtils.MethodRef<Object> asInterfaceMethod =
//                new RefUtils.MethodRef<Object>(stubClass, true, "asInterface", new Class[]{IBinder.class});
//
//        Object origin = asInterfaceMethod.invoke(null, new Object[]{defBinder});
//        Object newObj = null;
//
//        List<Class<?>> interfaces = getAllInterfaces(origin.getClass());
//
//        if (interfaces != null && interfaces.size() > 0) {
//            Class[] ifs = interfaces.toArray(new Class[interfaces.size()]);
//            newObj = Proxy.newProxyInstance(origin.getClass().getClassLoader(),
//                    ifs, new FlymePermissionProxy(origin));
//            new ServiceManagerHook(defBinder,origin, newObj).installBinderHook(serviceName);
//        }
//    }

    private static class ServiceManagerHook {
        private Object mOriginBinderInterface;
        private Object mOriginBinder;
        private Object mProxyBinder;
        ServiceManagerHook(Object originBinderInter, Object originBinder, Object proxyBinder) {
            mOriginBinderInterface = originBinderInter;
            mOriginBinder = originBinder;
            mProxyBinder = proxyBinder;
        }

        void install(String name) {
            if (mOriginBinderInterface == null
                    || mOriginBinder == null
                    || mProxyBinder == null) {
                Log.e(TAG, "installBinderHook " + name + " failed");
                return;
            }
            List<Class<?>> interfaces = getAllInterfaces(mOriginBinderInterface.getClass());
            if (interfaces != null && interfaces.size() > 0) {
                Class[] ifs = interfaces.toArray(new Class[interfaces.size()]);
                Object newObj = Proxy.newProxyInstance(mOriginBinderInterface.getClass().getClassLoader(),
                        ifs, new ServiceManagerProxy(mOriginBinder, mProxyBinder));

                sServiceCacheField.get(null).put(name, newObj);
                Log.d(TAG, "installBinderHook " + name + " success!");

            }
        }
    }

    private static class ServiceManagerProxy implements InvocationHandler {
        private Object mProxyServer;
        private Object mOriginServer;
        ServiceManagerProxy(Object originServer, Object proxyServer) {
            mOriginServer = originServer;
            mProxyServer = proxyServer;
        }

        @Override
        public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
            if (TextUtils.equals("queryLocalInterface", method.getName())) {
                Log.d(TAG, "queryLocalInterface return " + mProxyServer);
                return mProxyServer;
            }
            return method.invoke(mOriginServer, args);
        }
    }

    private static class FlymePermissionProxy extends BinderHook {

        private static final String className = "meizu.security.IFlymePermissionService$Stub";
        private static final String serviceName = "flyme_permission";

        FlymePermissionProxy() {
            super(className, serviceName);
        }

//        @Override
//        public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
//            Log.d(TAG, String.format("FlymePermissionProxy call %s %s",
//                    method.getName(), Arrays.toString(args)));
//            return method.invoke(proxy, args);
//        }
    }

    private static class AppopsManagerProxy extends BinderHook {

        private static final String className = "com.android.internal.app.IAppOpsService$Stub";
        private static final String serviceName = Context.APP_OPS_SERVICE;

        AppopsManagerProxy() {
            super(className, serviceName);
        }
//
//        @Override
//        public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
//            return method.invoke(proxy, args);
//        }
    }

    private static class PermissionManagerProxy extends BinderHook {

        private static final String className = "android.permission.IPermissionManager$Stub";
        private static final String serviceName = "permissionmgr";
        PermissionManagerProxy() {
            super(className, serviceName);
        }

        @Override
        protected void afterInstallBinder(Object origin, Object newObj) {
            RefUtils.FieldRef<Object> sPermissionManagerField = new RefUtils.FieldRef<Object>(
                    "android.app.ActivityThread",
                    true, "sPermissionManager");
            sPermissionManagerField.set(null, newObj);
            Log.d(TAG, String.format("install %s binder success", serviceName));
        }
    }

    private static class PackageManagerProxy extends BinderHook {

        private static final String className = "android.content.pm.IPackageManager$stub";
        private static final String serviceName = "android.content.pm.IPackageManager";
        PackageManagerProxy() {
            super(className, serviceName);
        }
    }

    private static class ForwardingOsProxy implements InvocationHandler {
        private Object mOrigin;
        ForwardingOsProxy(Object origin) {
            mOrigin = origin;
        }

        @Override
        public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
            Log.d(TAG, String.format("ForwardingOsProxy call %s %s",
                    method.getName(), Arrays.toString(args)));

//            if (TextUtils.equals(method.getName(), "remove")) {
//                RuntimeFake.testRemove((String) args[0]);
//                return method.invoke(mOrigin, "/data/data/com.example.demo/1.txt");
//            }
            return method.invoke(mOrigin, args);
        }
    }



    public static List<Class<?>> getAllInterfaces(final Class<?> cls) {
        if (cls == null) {
            return null;
        }
        final LinkedHashSet<Class<?>> interfacesFound = new LinkedHashSet<Class<?>>();
        getAllInterfaces(cls, interfacesFound);
        return new ArrayList<Class<?>>(interfacesFound);
    }

    private static void getAllInterfaces(Class<?> cls, final HashSet<Class<?>> interfacesFound) {
        while (cls != null) {
            final Class<?>[] interfaces = cls.getInterfaces();

            for (final Class<?> i : interfaces) {
                if (interfacesFound.add(i)) {
                    getAllInterfaces(i, interfacesFound);
                }
            }

            cls = cls.getSuperclass();
        }
    }
}
