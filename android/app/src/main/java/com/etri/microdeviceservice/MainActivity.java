package com.etri.microdeviceservice;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TableLayout;
import android.widget.Toast;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;

import java.io.File;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.MulticastSocket;
import java.net.UnknownHostException;
import java.nio.channels.DatagramChannel;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.List;
import android.content.Context;

public class MainActivity extends AppCompatActivity {
    Intent m_edit_intent;
    MulticastSocket m_ms = null;

    public class DeviceInfo {
        public int m_type;
        public String m_str_type;
        public DeviceControlService m_service;
        public Intent m_control_intent;
        public Intent m_edit_intent;
        public Button m_control_button;
        public Button m_edit_button;
    }

    UsbSerialPort m_port;
    HashMap<Integer, DeviceInfo> m_device_infos;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Toast myToast;

        init();

        if(initSerial() < 0){
            myToast = Toast.makeText(getApplicationContext(), "Serial Port init failed.", Toast.LENGTH_LONG);
            myToast.show();
            //return;
        }
        else {
            myToast = Toast.makeText(getApplicationContext(), "Serial Port init success.", Toast.LENGTH_LONG);
            myToast.show();
        }

        setMulticast();
    }

    /*
    void createMulticastSocket(){
        InetAddress ia = null;
        int port = 57777;

        try {
            m_ms = new MulticastSocket(port);
            ia = InetAddress.getByName("224.0.1.2");
            m_ms.joinGroup(ia);

            ia = InetAddress.getByName("224.0.1.1");
            m_ms.joinGroup(ia);


        } catch (Exception e) {
            android.util.Log.i("**my", "create socket failed...............");
            e.printStackTrace();
        }

    }

    void sendMulticast(){
        Thread thread = new Thread(new Runnable() {

            @Override
            public void run() {
                while(true) {
                    try {
                        Thread.sleep(1000);

                        InetAddress ia = null;
                        int port = 17777;
                        byte[] buffer = {(byte) 0xf7, (byte) 0x0, (byte) 0x1f, (byte) 0x81, (byte) 0x05, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x6c, (byte) 0x08};

                        try {
                            //ia = InetAddress.getByName("224.0.1.2");
                            ia = InetAddress.getByName("255.255.255.255");
                            //ia = InetAddress.getByName("192.168.0.101");
                            DatagramPacket dp = new DatagramPacket(buffer, buffer.length, ia, port);
                            m_ms.send(dp);
                            android.util.Log.i("**my", "send bytes...............");
                            android.util.Log.i("**my", "send bytes...............");
                            android.util.Log.i("**my", "send bytes...............");

                        } catch (Exception e) {
                            android.util.Log.i("**my", "send multicast failed...............");
                            e.printStackTrace();
                        }
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            }
        });

        thread.start();

    }
     */

    int setMulticast(){

        WifiManager wifi = (WifiManager)getApplicationContext().getSystemService( Context.WIFI_SERVICE );
        if(wifi != null){
            WifiManager.MulticastLock lock = wifi.createMulticastLock("Log_Tag");
            lock.acquire();
            android.util.Log.i("**my", "set multicast lock");
        }

        return 0;
    }

    int initSerial(){
        android.util.Log.i("**my", "serial init start");

        UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);

        List<UsbSerialDriver> availableDrivers = UsbSerialProber.getDefaultProber().findAllDrivers(manager);
        if (availableDrivers.isEmpty()) {
            return -1;
        }

        android.util.Log.i("**my", "serial size: " + availableDrivers.size());

        // Open a connection to the first available driver.
        UsbSerialDriver driver = availableDrivers.get(0);
        UsbDeviceConnection connection = manager.openDevice(driver.getDevice());
        if (connection == null) {
            // add UsbManager.requestPermission(driver.getDevice(), ..) handling here
            return -1;
        }

        m_port = driver.getPorts().get(0); // Most devices have just one port (port 0)
        try {
            m_port.open(connection);
            m_port.setParameters(38400, 8, 1, UsbSerialPort.PARITY_NONE);
            //m_port.setParameters(115200, 8, 1, UsbSerialPort.PARITY_NONE);
        } catch (Exception e) {
            e.printStackTrace();
            return -1;
        }

        DeviceService.getInstance().setSerialPort(m_port);;

        return 0;
    }

    private int init(){
        android.util.Log.i("**my", "start of init");
        m_device_infos = new HashMap<Integer, DeviceInfo>();
        setDeviceList();

        android.util.Log.i("**my", "data path: " + getApplicationContext().getFilesDir().getAbsolutePath());
        DeviceService.getInstance().setAssetManager(getApplicationContext().getAssets());
        DeviceService.getInstance().setBaseDir(getApplicationContext().getFilesDir().getAbsolutePath());
        DeviceService.getInstance().copyAssetFolder(getApplicationContext(), "", getApplicationContext().getFilesDir().getAbsolutePath());

        m_edit_intent = new Intent(getApplicationContext(), EditConfigurationActivity.class);

        android.util.Log.i("**my", "end of init");

        return 0;
    }

    public class ButtonControlListener implements Button.OnClickListener {
        int m_type;

        public ButtonControlListener(int type){
            m_type = type;
        }

        @Override
        public void onClick(View v) {
           String button_text;
            DeviceInfo device_info;

            device_info = m_device_infos.get(m_type);
            if(device_info == null){
                android.util.Log.e("**my", "there is no device type: " + m_type);
                return;
            }
            android.util.Log.i("**my", "control button clicked: " + m_type);

            button_text = device_info.m_control_button.getText().toString();

            if(button_text.compareToIgnoreCase("start") == 0) {
                android.util.Log.i("**my", "start service: " + device_info.m_str_type);
                device_info.m_control_intent.putExtra("type", device_info.m_type);
                startService(device_info.m_control_intent);
                device_info.m_control_button.setText("started");
            }
            /*
            else{
                android.util.Log.i("**my", "stop service: " + device_info.m_str_type);
                device_info.m_control_intent.putExtra("type", device_info.m_type);
                stopService(device_info.m_control_intent);
                device_info.m_control_button.setText("start");
            }
             */
        }
    }

    public class ButtonEditListener implements Button.OnClickListener {
        int m_type;

        public ButtonEditListener(int type){
            m_type = type;
        }

        @Override
        public void onClick(View v) {
            DeviceInfo device_info;

            device_info = m_device_infos.get(m_type);
            if(device_info == null){
                android.util.Log.e("**my", "there is no device type: " + m_type);
                return;
            }
            android.util.Log.i("**my", "edit button clicked: " + m_type);

            device_info.m_edit_intent.putExtra("type", device_info.m_type);
            startActivity(device_info.m_edit_intent);
         }
    }

    private void setDeviceList(){
        DeviceInfo device_info;

        device_info = new DeviceInfo();
        device_info.m_type = DeviceService.FORWARDER_DEVICE_ID;
        device_info.m_str_type = "forwarder";
        device_info.m_control_intent = new Intent(this, DeviceControlService.class);
        device_info.m_edit_intent = new Intent(this, EditConfigurationActivity.class);
        device_info.m_control_button = null;
        device_info.m_edit_button = findViewById(R.id.forwarderEditButton);
        device_info.m_edit_button.setOnClickListener(new ButtonEditListener(device_info.m_type));
        m_device_infos.put(device_info.m_type, device_info);

        device_info = new DeviceInfo();
        device_info.m_type = DeviceService.CONTROLLER_DEVICE_ID;
        device_info.m_str_type = "controller";
        device_info.m_control_intent = new Intent(this, DeviceControlService.class);
        device_info.m_edit_intent = new Intent(this, EditConfigurationActivity.class);
        device_info.m_control_button = findViewById(R.id.controllerButton);
        device_info.m_edit_button = findViewById(R.id.controllerEditButton);
        device_info.m_control_button.setOnClickListener(new ButtonControlListener(device_info.m_type));
        device_info.m_edit_button.setOnClickListener(new ButtonEditListener(device_info.m_type));
        m_device_infos.put(device_info.m_type, device_info);

        device_info = new DeviceInfo();
        device_info.m_type = DeviceService.SYSTEMAIRCON_DEVICE_ID;
        device_info.m_str_type = "systemAircon";
        device_info.m_control_intent = new Intent(this, DeviceControlService.class);
        device_info.m_edit_intent = new Intent(this, EditConfigurationActivity.class);
        device_info.m_control_button = findViewById(R.id.systemAirconButton);
        device_info.m_edit_button = findViewById(R.id.systemAirconEditButton);
        device_info.m_control_button.setOnClickListener(new ButtonControlListener(device_info.m_type));
        device_info.m_edit_button.setOnClickListener(new ButtonEditListener(device_info.m_type));
        m_device_infos.put(device_info.m_type, device_info);

        device_info = new DeviceInfo();
        device_info.m_type = DeviceService.LIGHT_DEVICE_ID;
        device_info.m_str_type = "light";
        device_info.m_control_intent = new Intent(this, DeviceControlService.class);
        device_info.m_edit_intent = new Intent(this, EditConfigurationActivity.class);
        device_info.m_control_button = findViewById(R.id.lightButton);
        device_info.m_edit_button = findViewById(R.id.lightEditButton);
        device_info.m_control_button.setOnClickListener(new ButtonControlListener(device_info.m_type));
        device_info.m_edit_button.setOnClickListener(new ButtonEditListener(device_info.m_type));
        m_device_infos.put(device_info.m_type, device_info);

        device_info = new DeviceInfo();
        device_info.m_type = DeviceService.GASVALVE_DEVICE_ID;
        device_info.m_str_type = "gasValve";
        device_info.m_control_intent = new Intent(this, DeviceControlService.class);
        device_info.m_edit_intent = new Intent(this, EditConfigurationActivity.class);
        device_info.m_control_button = findViewById(R.id.gasValveButton);
        device_info.m_edit_button = findViewById(R.id.gasValveEditButton);
        device_info.m_control_button.setOnClickListener(new ButtonControlListener(device_info.m_type));
        device_info.m_edit_button.setOnClickListener(new ButtonEditListener(device_info.m_type));
        m_device_infos.put(device_info.m_type, device_info);

        device_info = new DeviceInfo();
        device_info.m_type = DeviceService.CURTAIN_DEVICE_ID;
        device_info.m_str_type = "curtain";
        device_info.m_control_intent = new Intent(this, DeviceControlService.class);
        device_info.m_edit_intent = new Intent(this, EditConfigurationActivity.class);
        device_info.m_control_button = findViewById(R.id.curtainButton);
        device_info.m_edit_button = findViewById(R.id.curtainEditButton);
        device_info.m_control_button.setOnClickListener(new ButtonControlListener(device_info.m_type));
        device_info.m_edit_button.setOnClickListener(new ButtonEditListener(device_info.m_type));
        m_device_infos.put(device_info.m_type, device_info);

        device_info = new DeviceInfo();
        device_info.m_type = DeviceService.REMOTEINSPECTOR_DEVICE_ID;
        device_info.m_str_type = "remoteInspector";
        device_info.m_control_intent = new Intent(this, DeviceControlService.class);
        device_info.m_edit_intent = new Intent(this, EditConfigurationActivity.class);
        device_info.m_control_button = findViewById(R.id.remoteInspectorButton);
        device_info.m_edit_button = findViewById(R.id.remoteInspectorEditButton);
        device_info.m_control_button.setOnClickListener(new ButtonControlListener(device_info.m_type));
        device_info.m_edit_button.setOnClickListener(new ButtonEditListener(device_info.m_type));
        m_device_infos.put(device_info.m_type, device_info);

        device_info = new DeviceInfo();
        device_info.m_type = DeviceService.DOORLOCK_DEVICE_ID;
        device_info.m_str_type = "doorLock";
        device_info.m_control_intent = new Intent(this, DeviceControlService.class);
        device_info.m_edit_intent = new Intent(this, EditConfigurationActivity.class);
        device_info.m_control_button = findViewById(R.id.doorLockButton);
        device_info.m_edit_button = findViewById(R.id.doorLockEditButton);
        device_info.m_control_button.setOnClickListener(new ButtonControlListener(device_info.m_type));
        device_info.m_edit_button.setOnClickListener(new ButtonEditListener(device_info.m_type));
        m_device_infos.put(device_info.m_type, device_info);

        device_info = new DeviceInfo();
        device_info.m_type = DeviceService.VANTILATOR_DEVICE_ID;
        device_info.m_str_type = "vantailator";
        device_info.m_control_intent = new Intent(this, DeviceControlService.class);
        device_info.m_edit_intent = new Intent(this, EditConfigurationActivity.class);
        device_info.m_control_button = findViewById(R.id.vantilatorButton);
        device_info.m_edit_button = findViewById(R.id.vantilatorEditButton);
        device_info.m_control_button.setOnClickListener(new ButtonControlListener(device_info.m_type));
        device_info.m_edit_button.setOnClickListener(new ButtonEditListener(device_info.m_type));
        m_device_infos.put(device_info.m_type, device_info);

        device_info = new DeviceInfo();
        device_info.m_type = DeviceService.BREAKER_DEVICE_ID;
        device_info.m_str_type = "breaker";
        device_info.m_control_intent = new Intent(this, DeviceControlService.class);
        device_info.m_edit_intent = new Intent(this, EditConfigurationActivity.class);
        device_info.m_control_button = findViewById(R.id.breakerButton);
        device_info.m_edit_button = findViewById(R.id.breakerEditButton);
        device_info.m_control_button.setOnClickListener(new ButtonControlListener(device_info.m_type));
        device_info.m_edit_button.setOnClickListener(new ButtonEditListener(device_info.m_type));
        m_device_infos.put(device_info.m_type, device_info);

        device_info = new DeviceInfo();
        device_info.m_type = DeviceService.PREVENTCRIMEEXT_DEVICE_ID;
        device_info.m_str_type = "preventCrimeExt";
        device_info.m_control_intent = new Intent(this, DeviceControlService.class);
        device_info.m_edit_intent = new Intent(this, EditConfigurationActivity.class);
        device_info.m_control_button = findViewById(R.id.preventCrimeExtButton);
        device_info.m_edit_button = findViewById(R.id.preventCrimeExtEditButton);
        device_info.m_control_button.setOnClickListener(new ButtonControlListener(device_info.m_type));
        device_info.m_edit_button.setOnClickListener(new ButtonEditListener(device_info.m_type));
        m_device_infos.put(device_info.m_type, device_info);

        device_info = new DeviceInfo();
        device_info.m_type = DeviceService.BOILER_DEVICE_ID;
        device_info.m_str_type = "boiler";
        device_info.m_control_intent = new Intent(this, DeviceControlService.class);
        device_info.m_edit_intent = new Intent(this, EditConfigurationActivity.class);
        device_info.m_control_button = findViewById(R.id.boilerButton);
        device_info.m_edit_button = findViewById(R.id.boilerEditButton);
        device_info.m_control_button.setOnClickListener(new ButtonControlListener(device_info.m_type));
        device_info.m_edit_button.setOnClickListener(new ButtonEditListener(device_info.m_type));
        m_device_infos.put(device_info.m_type, device_info);

        device_info = new DeviceInfo();
        device_info.m_type = DeviceService.TEMPERATURECONTROLLER_DEVICE_ID;
        device_info.m_str_type = "temperatureController";
        device_info.m_control_intent = new Intent(this, DeviceControlService.class);
        device_info.m_edit_intent = new Intent(this, EditConfigurationActivity.class);
        device_info.m_control_button = findViewById(R.id.temperatureControllerButton);
        device_info.m_edit_button = findViewById(R.id.temperatureControllerEditButton);
        device_info.m_control_button.setOnClickListener(new ButtonControlListener(device_info.m_type));
        device_info.m_edit_button.setOnClickListener(new ButtonEditListener(device_info.m_type));
        m_device_infos.put(device_info.m_type, device_info);

        device_info = new DeviceInfo();
        device_info.m_type = DeviceService.POWERGATE_DEVICE_ID;
        device_info.m_str_type = "powerGate";
        device_info.m_control_intent = new Intent(this, DeviceControlService.class);
        device_info.m_edit_intent = new Intent(this, EditConfigurationActivity.class);
        device_info.m_control_button = findViewById(R.id.powerGateButton);
        device_info.m_edit_button = findViewById(R.id.powerGateEditButton);
        device_info.m_control_button.setOnClickListener(new ButtonControlListener(device_info.m_type));
        device_info.m_edit_button.setOnClickListener(new ButtonEditListener(device_info.m_type));
        m_device_infos.put(device_info.m_type, device_info);

        return;
    }
}