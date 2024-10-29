package com.etri.microdeviceservice;

import android.content.Context;
import android.content.res.AssetManager;

import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.util.SerialInputOutputManager;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Set;

public class DeviceService implements SerialInputOutputManager.Listener {
    public static final int CONTROLLER_DEVICE_ID           = 0xE8;
    public static final int FORWARDER_DEVICE_ID                   = 0XE9;

    public static final int SYSTEMAIRCON_DEVICE_ID         = 0X02;
    //public static final int MICROWAVEOVEN_DEVICE_ID        = 0X04;
    //public static final int DISHWASHER_DEVICE_ID           = 0X09;
    //public static final int DRUMWASHER_DEVICE_ID           = 0X0A;
    public static final int LIGHT_DEVICE_ID                = 0X0E;
    public static final int GASVALVE_DEVICE_ID             = 0X12;
    public static final int CURTAIN_DEVICE_ID              = 0X13;
    public static final int REMOTEINSPECTOR_DEVICE_ID      = 0X30;
    public static final int DOORLOCK_DEVICE_ID             = 0X31;
    public static final int VANTILATOR_DEVICE_ID           = 0X32;
    public static final int BREAKER_DEVICE_ID              = 0X33;
    public static final int PREVENTCRIMEEXT_DEVICE_ID      = 0X34;
    public static final int BOILER_DEVICE_ID               = 0X35;
    public static final int TEMPERATURECONTROLLER_DEVICE_ID = 0X36;
    //public static final int ZIGBEE_DEVICE_ID               = 0X37;
    //public static final int POWERMETER_DEVICE_ID           = 0X38;
    public static final int POWERGATE_DEVICE_ID            = 0X39;

    String CONTROLLER_DEVICE           = "controller.json";
    String FORWARDER_DEVICE            = "forwarder.json";

    String SYSTEMAIRCON_DEVICE         = "system_aircon.json";
    //static String MICROWAVEOVEN_DEVICE        = "";
    //static String DISHWASHER_DEVICE           = "";
    //static String DRUMWASHER_DEVICE           = "";
    String LIGHT_DEVICE                = "light.json";
    String GASVALVE_DEVICE             = "gasvalve.json";
    String CURTAIN_DEVICE              = "curtain.json";
    String REMOTEINSPECTOR_DEVICE      = "remote_inspector.json";
    String DOORLOCK_DEVICE             = "doorlock.json";
    String VANTILATOR_DEVICE           = "vantilator.json";
    String BREAKER_DEVICE              = "breaker.json";
    String PREVENTCRIMEEXT_DEVICE      = "prevent_crime_ext.json";
    String BOILER_DEVICE               = "boiler.json";
    String TEMPERATURECONTROLLER_DEVICE = "temperature_controller.json";
    //String ZIGBEE_DEVICE               = "";
    //String POWERMETER_DEVICE           = "";
    String POWERGATE_DEVICE            = "powergate.json";

    AssetManager m_asset_manager;
    HashMap<Integer, String> m_device_path;

    SerialDataPoller m_poller;

    static Object m_mutex = new Object();
    static DeviceService m_instance = null;
    static UsbSerialPort m_port;

    public class SerialDataPoller extends Thread{

        @Override
        public void run() {
            byte[] data;

            while(true){
                data = pollSerialData_internal();
                if(data == null){
                    android.util.Log.e("**my", "no serial data");
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e){
                    }
                    continue;
                }

                android.util.Log.e("**my", "got serial data");
                writeSerial(data);
            }
        }
    }

    public static String getLocalIpAddress() {
        try {
            for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();) {
                NetworkInterface intf = en.nextElement();
                for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements();) {
                    InetAddress inetAddress = enumIpAddr.nextElement();
                    if (!inetAddress.isLoopbackAddress() && inetAddress instanceof Inet4Address) {
                        return inetAddress.getHostAddress();
                    }
                }
            }
        } catch (SocketException ex) {
            ex.printStackTrace();
        }
        return null;
    }

    private DeviceService() {
        m_device_path = new HashMap<Integer, String>();

        m_device_path.put(CONTROLLER_DEVICE_ID, CONTROLLER_DEVICE);
        m_device_path.put(SYSTEMAIRCON_DEVICE_ID, SYSTEMAIRCON_DEVICE);
        m_device_path.put(LIGHT_DEVICE_ID, LIGHT_DEVICE);
        m_device_path.put(GASVALVE_DEVICE_ID, GASVALVE_DEVICE);
        m_device_path.put(CURTAIN_DEVICE_ID, CURTAIN_DEVICE);
        m_device_path.put(REMOTEINSPECTOR_DEVICE_ID, REMOTEINSPECTOR_DEVICE);
        m_device_path.put(DOORLOCK_DEVICE_ID, DOORLOCK_DEVICE);
        m_device_path.put(VANTILATOR_DEVICE_ID, VANTILATOR_DEVICE);
        m_device_path.put(BREAKER_DEVICE_ID, BREAKER_DEVICE);
        m_device_path.put(PREVENTCRIMEEXT_DEVICE_ID, PREVENTCRIMEEXT_DEVICE);
        m_device_path.put(BOILER_DEVICE_ID, BOILER_DEVICE);
        m_device_path.put(TEMPERATURECONTROLLER_DEVICE_ID, TEMPERATURECONTROLLER_DEVICE);
        m_device_path.put(POWERGATE_DEVICE_ID, POWERGATE_DEVICE);
        m_device_path.put(FORWARDER_DEVICE_ID, FORWARDER_DEVICE);
    }

    public static DeviceService getInstance(){
        synchronized (m_mutex){
            if(m_instance == null){
                m_instance = new DeviceService();
                if(m_instance.init() < 0){
                    android.util.Log.e("**my", "DeviceService init failed.");
                    m_instance = null;
                }
            }
        }
        return m_instance;
    }

    static {
        android.util.Log.i("**my", "load jni library.");
        System.loadLibrary("microdeviceservice");
        m_port = null;
    }

    private int init() {
        String addr;

        cppClearMyAddress();
        addr = getLocalIpAddress();
        android.util.Log.i("**my", "get my addr: " + addr);
        cppAddMyAddress(addr);

        m_poller = new SerialDataPoller();
        m_poller.start();

        return 0;
    }

    public void setAssetManager(AssetManager asset_manager){
        m_asset_manager = asset_manager;
    }

    public void setSerialPort(UsbSerialPort port){

        m_port = port;
        SerialInputOutputManager usbIoManager;

        usbIoManager = new SerialInputOutputManager(m_port, this);
        usbIoManager.start();
     }

    public static boolean copyAssetFolder(Context context, String srcName, String dstName) {
        //android.util.Log.i("**my", "copy directory: " + srcName);
        try {
            boolean result = true;
            String fileList[] = context.getAssets().list(srcName);
            if (fileList == null) return false;

            if (fileList.length == 0) {
                result = copyAssetFile(context, srcName, dstName);
            } else {
                File file = new File(dstName);
                result = file.mkdirs();
                for (String filename : fileList) {
                    if(srcName.length() == 0){
                        result &= copyAssetFolder(context, filename, dstName + File.separator + filename);
                    }
                    else{
                        result &= copyAssetFolder(context, srcName + File.separator + filename, dstName + File.separator + filename);
                    }
                }
            }
            return result;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    public static boolean copyAssetFile(Context context, String srcName, String dstName) {
        //android.util.Log.i("**my", "copy file: " + srcName);

        try {
            InputStream in = context.getAssets().open(srcName);
            File outFile = new File(dstName);
            OutputStream out = new FileOutputStream(outFile);
            byte[] buffer = new byte[1024];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
            in.close();
            out.close();
            return true;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    private String readStream(InputStream input_stream){
        int length;
        byte[] buffer;
        String str;

        str = null;

        try {
            length = input_stream.available();
            buffer = new byte[length];
            input_stream.read(buffer, 0, length);
            input_stream.close();

            str = new String(buffer);
        } catch(Exception e){
            return null;
        }

        return str;
    }

    private int writeString(String str, String path){
        File file;
        byte[] bytes;

        file = new File(path);
        bytes = str.getBytes();

        try {
            FileOutputStream outputStream = new FileOutputStream(file);
            outputStream.write(bytes);
            outputStream.close();
        } catch (Exception e) {
            return -1;
        }

        return 0;
    }

    public int writeString(String str, int type) {
        File file;
        String path;
        byte[] bytes;

        path = m_device_path.get(type);
        if(path == null){
            android.util.Log.i("**my","invalid type: " + type);
            return -1;
        }

        file = new File(path);
        bytes = str.getBytes();

        try {
            FileOutputStream outputStream = new FileOutputStream(file);
            outputStream.write(bytes);
            outputStream.close();
        } catch (Exception e) {
            return -1;
        }

        return 0;
    }

    public void setBaseDir(String base_path){
        InputStream input_stream;
        String str_json;
        String path;
        String asset_name;
        File file;
        Iterator<Integer> key_iter;
        int key;

        android.util.Log.i("**my", "set base path: " + base_path);

        key_iter = m_device_path.keySet().iterator();

        while(key_iter.hasNext()){
            key = key_iter.next();

            asset_name = m_device_path.get(key);
            if(asset_name == null){
                continue;
            }

            path = base_path + "/" + asset_name;

            /*
            file = new File(path);
            file.delete();
            if (file.exists() == false) {
                str_json = null;
                try {
                    input_stream = m_asset_manager.open(asset_name);
                    str_json = readStream(input_stream);
                } catch (Exception e) {
                    android.util.Log.e("**my", "asset not found: " + asset_name);
                }
                if (str_json != null) {
                    writeString(str_json, path);
                }
            }
            */

            m_device_path.put(key, path);
        }

        cppSetBaseFolder(base_path);

        str_json = getJsonString(FORWARDER_DEVICE_ID);
        android.util.Log.e("**my","gogo. \n" + str_json);
        if(str_json == null){
            android.util.Log.e("**my","there is no configuration for Forwarder.");
        }
        else{
            if(cppStartForwarder(str_json) < 0){
                android.util.Log.e("**my","failed to start forwarder.");
            }
        }

        return;
    }

    public String getJsonString(int type){
        String path;
        String str_json;
        InputStream input_stream;

        path = m_device_path.get(type);
        android.util.Log.i("**my","start device: " + type + ", " + path);

        if(path == null){
            android.util.Log.i("**my","invalid type: " + type);
            return null;
        }

        try {
            input_stream = new FileInputStream(new File(path));
        } catch(Exception e){
            android.util.Log.i("**my","file not found: " + path);
            return null;
        }
        str_json = readStream(input_stream);

        return str_json;
    }

    public int startDevice(int type){
        int ret;
        String str_json;
        String path;
        InputStream input_stream;

        ret = 0;
        str_json = null;

        path = m_device_path.get(type);
        android.util.Log.i("**my","start device: " + type + ", " + path);

        if(path == null){
            android.util.Log.i("**my","invalid type: " + type);
            return -1;
        }

        try {
            input_stream = new FileInputStream(new File(path));
        } catch(Exception e){
            android.util.Log.i("**my","file not found: " + path);
            return -1;
        }
        str_json = readStream(input_stream);

        ret = cppStartDevice(type, str_json);

        if(ret < 0){
            android.util.Log.i("**my","start device failed.");
        }
        return ret;
    }

    public int writeSerial(byte[] data){
        int ret = -1;

        if(m_port == null){
            android.util.Log.e("**my","serial port is null.");
            return -1;
        }
        try{
            m_port.write(data, 0);
            android.util.Log.e("**my","write to serial: " + byteArrayToHex(data));
        } catch(Exception e){
            android.util.Log.e("**my","write failed.");
            return -1;
        }
        return ret;
    }

    @Override
    public void onNewData(byte[] data) {
        android.util.Log.e("**my", "onNewData from serial: " + data.length + ": " + byteArrayToHex(data));
        cpponNewData(data);
    }

    public byte[] pollSerialData_internal() {
        return pollSerialData();
    }

    @Override
    public void onRunError(Exception e) {
        e.printStackTrace();
    }

    public void stopDevice(int type){
        cppStopDevice(type);
    }

    public static String byteArrayToHex(byte[] a) {
        StringBuilder sb = new StringBuilder(a.length * 2);
        for(byte b: a)
            sb.append(String.format("%02x ", b));
        return sb.toString();
    }


    public native void cppClearMyAddress();
    public native void cppAddMyAddress(String addr);
    public native void cppSetBaseFolder(String path);
    public native int cppStartForwarder(String str_json);
    public native int cppStartDevice(int type, String str_json);
    public native void cppStopDevice(int type);
    public native void cpponNewData(byte[] data);
    public native byte[] pollSerialData();

}
