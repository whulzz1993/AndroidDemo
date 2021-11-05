package com.example.demo.test;

import android.util.Log;

import androidx.annotation.Keep;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import okhttp3.Interceptor;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.ResponseBody;
import retrofit2.Call;
import retrofit2.Invocation;
import retrofit2.Response;
import retrofit2.Retrofit;
import retrofit2.converter.gson.GsonConverterFactory;
import retrofit2.http.Body;
import retrofit2.http.GET;
import retrofit2.http.Header;
import retrofit2.http.HeaderMap;
import retrofit2.http.POST;
import retrofit2.http.QueryMap;

public class NetTest {

    public static class StackMsg {
        @Keep
        final String classname;
        @Keep
        final String funcname;
        @Keep
        final String stack;
        public StackMsg(String classname, String func, String stack) {
            this.classname = classname;
            this.funcname = func;
            this.stack = stack;
        }
    }

    interface ServiceApi {
        @GET("/robots.txt")
        Call<ResponseBody> robots();

        @POST("/stack_upload")
        Call<ResponseBody> postStack(@Header("appid") String appid, @Body List<StackMsg> stacks);
    }

    interface DownloadApi {
        @GET("invoke/getConfig")
        Call<ResponseBody> download(@HeaderMap Map<String, String> headers, @QueryMap Map<String, String> params);
    }

    static final class InvocationLogger implements Interceptor {
        @Override
        public okhttp3.Response intercept(Chain chain) throws IOException {
            Request request = chain.request();
            long startNanos = System.nanoTime();
            okhttp3.Response response = chain.proceed(request);
            long elapsedNanos = System.nanoTime() - startNanos;

            Invocation invocation = request.tag(Invocation.class);
            if (invocation != null) {
                System.out.printf(
                        "%s.%s %s HTTP %s (%.0f ms)%n",
                        invocation.method().getDeclaringClass().getSimpleName(),
                        invocation.method().getName(),
                        invocation.arguments(),
                        response.code(),
                        elapsedNanos / 1_000_000.0);
            }

            return response;
        }
    }

    public static void test() {
//        testUpload();

        /******test download BEGIN***/
        Map<String, String> params = new HashMap<String, String>();
        params.put("type", "2");
        params.put("appid", "1128");
        params.put("os", "2");
        InputStream is = testDownload(new HashMap<String, String>(), params);

        if (is != null) {
            FileOutputStream fos = null;
            try {
                fos = new FileOutputStream("/data/data/com.example.demo/download.tmp");
                byte[] bytes = new byte[1024];
                int n = 0;
                while ((n = is.read(bytes)) > 0) {
                    fos.write(bytes, 0, n);
                }
                fos.flush();
                Log.d("whulzz", "download test success!");
            } catch (Exception e) {
                e.printStackTrace();
            } finally {
                if (fos != null) {
                    try {
                        fos.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }

                try {
                    is.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        /******test download END***/
    }

    private static void testUpload() {
        InvocationLogger invocationLogger = new InvocationLogger();
        OkHttpClient okHttpClient = new OkHttpClient.Builder().addInterceptor(invocationLogger).build();

        Retrofit retrofit = new Retrofit.Builder()
                .baseUrl("https://xxx.xxx.xxx/")
                .addConverterFactory(GsonConverterFactory.create())
                .callFactory(okHttpClient).build();


        ServiceApi api = retrofit.create(ServiceApi.class);
        try {
            List<StackMsg> stacks = new ArrayList<>();
            stacks.add(new StackMsg("com.example.demo.Test",
                    "test", "com.example.test"));

            stacks.add(new StackMsg("com.example.demo.Test",
                    "test", "com.example.test"));

            Response response = api.postStack("1111", stacks).execute();

            System.out.println(response.body());

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static InputStream testDownload(Map<String, String> headers, Map<String, String> params) {
        Retrofit retrofit = null;
        if (true) {
            InvocationLogger invocationLogger = new InvocationLogger();
            OkHttpClient okHttpClient = new OkHttpClient.Builder().addInterceptor(invocationLogger).build();
            retrofit = new Retrofit.Builder()
                    .baseUrl("https://xxx.xxx.xxx/")
                    .addConverterFactory(GsonConverterFactory.create()).callFactory(okHttpClient).build();
        }

        DownloadApi api = retrofit.create(DownloadApi.class);

        try {
            Response<ResponseBody> response = api.download(headers, params).execute();
            return response.body().byteStream();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
}
