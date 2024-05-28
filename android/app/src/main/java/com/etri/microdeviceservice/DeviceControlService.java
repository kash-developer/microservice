package com.etri.microdeviceservice;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;
import android.widget.Button;

import androidx.annotation.Nullable;

public class DeviceControlService extends Service {
    int m_type;

    public DeviceControlService(){
        android.util.Log.i("**my","DeviceControlService()");
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate(){
        super.onCreate();
    }

    @Override
    public void onDestroy() {
        android.util.Log.i("**my","onDestroy()");
        DeviceService.getInstance().stopDevice(DeviceService.LIGHT_DEVICE_ID);
        super.onDestroy();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        android.util.Log.i("**my","onStartCommand()");

        m_type = intent.getIntExtra("type", -1);

        if(DeviceService.getInstance().startDevice(m_type) < 0){
            android.util.Log.i("**my","start device failed.");
        }

         return super.onStartCommand(intent, flags, startId);
    }
}
