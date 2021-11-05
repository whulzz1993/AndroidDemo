package com.example.demo.test;

import android.accounts.AccountManager;
import android.content.ClipboardManager;
import android.content.Context;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.net.ConnectivityManager;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.telephony.TelephonyManager;
import android.util.Log;

import androidx.annotation.NonNull;

import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.Collections;
import java.util.Enumeration;
import java.util.List;

public class PrivacyApiTest {

    public static void test(Context context) {
        //getInstalledApplications
        context.getPackageManager().getInstalledApplications(0);

        //TelephonyManager
        TelephonyManager telephoneManager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
        try {
            telephoneManager.getLine1Number();
        } catch (Exception e) {
            e.printStackTrace();
        }
        try {
            telephoneManager.getCellLocation();
        } catch (Exception e) {
            e.printStackTrace();
        }
        try {
            telephoneManager.getAllCellInfo();
        } catch (Exception e) {
            e.printStackTrace();
        }
        try {
            telephoneManager.getDeviceId();
        } catch (Exception e) {
            e.printStackTrace();
        }
        try {
            telephoneManager.getSubscriberId();
        } catch (Exception e) {
            e.printStackTrace();
        }
        try {
            telephoneManager.getSimSerialNumber();
        } catch (Exception e) {
            e.printStackTrace();
        }
        try {
            telephoneManager.getVoiceMailNumber();
        } catch (Exception e) {
            e.printStackTrace();
        }

        //WifiManager
        WifiManager wifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);

        try {
            wifiManager.getScanResults();
        } catch (Exception e) {
            e.printStackTrace();
        }

        try {
            WifiInfo wifiInfo = wifiManager.getConnectionInfo();
            wifiInfo.getSSID();
            wifiInfo.getBSSID();
            wifiInfo.getMacAddress();
            wifiInfo.getIpAddress();
        } catch (Exception e) {
            e.printStackTrace();
        }

        //LocationManager
        LocationManager locationManager = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
        try {
            List<String> providers = locationManager.getProviders(true);
            for (String provider : providers) {
                locationManager.getLastKnownLocation(provider);
                locationManager.requestSingleUpdate(provider, new LocationListener() {
                    @Override
                    public void onLocationChanged(@NonNull Location location) {
                        Log.d("PrivacyApiTest", "onLocationChanged: " + location);
                    }
                }, null);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        //java.net....
        try {
            //IPV4
            InetAddress.getByName("1.2.3.4").getHostAddress();


            byte[] eAddr = {
                    (byte) 0x00, (byte) 0x01, (byte) 0x02, (byte) 0x03,
                    (byte) 0x04, (byte) 0x05, (byte) 0x06, (byte) 0x07,
                    (byte) 0x08, (byte) 0x09, (byte) 0x0a, (byte) 0x0b,
                    (byte) 0x0c, (byte) 0x0d, (byte) 0x0e, (byte) 0x0f
            };
            //IPV6
            Inet6Address.getByAddress(eAddr).getHostAddress();
        } catch (UnknownHostException e) {
            e.printStackTrace();
        }

        //Clipboard
        ClipboardManager clipboardManager = (ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
        try {
            clipboardManager.getPrimaryClip();
        } catch (Exception e) {
            e.printStackTrace();
        }

        //AccountManager
        AccountManager accountManager = (AccountManager) context.getSystemService(Context.ACCOUNT_SERVICE);
        try {
            accountManager.getAccounts();
            accountManager.getAccountsByType(null);
            //accountManager.getAccountsByTypeAndFeatures();
        } catch (Exception e) {
            e.printStackTrace();
        }

        try {
            Enumeration<NetworkInterface> networkInterfaceEnumeration =
                    NetworkInterface.getNetworkInterfaces();

            for (NetworkInterface nif : Collections.list(networkInterfaceEnumeration)) {
                nif.getHardwareAddress();
            }

        } catch (Exception e) {
            e.printStackTrace();
        }

        ConnectivityManager connectivityManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        try {
            connectivityManager.getAllNetworkInfo();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
