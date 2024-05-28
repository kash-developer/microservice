package com.etri.microdeviceservice;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

public class EditConfigurationActivity extends AppCompatActivity implements View.OnClickListener {

    TextView m_type_view;
    TextView m_edit;
    Button m_ok_button;

    int m_type;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_edit_configuration);
        init();
    }

    private void init(){
        int type;
        String str_json;

        m_type = getIntent().getIntExtra("type", -1);

        android.util.Log.i("**my", "configuration edit init: " + m_type);

        m_type_view = (TextView)findViewById(R.id.deviceTypeText);
        m_type_view.setText("");

        m_edit = (TextView)findViewById(R.id.editConfig);
        m_edit.setFocusable(true);
        m_edit.setEnabled(true);
        m_edit.setClickable(true);
        m_edit.setFocusableInTouchMode(true);

        m_ok_button = (Button)findViewById(R.id.editOk);
        m_ok_button.setOnClickListener(this);

        str_json = DeviceService.getInstance().getJsonString(m_type);
        if(str_json == null){
            m_edit.setText("");
        }
        else{
            m_edit.setText(str_json);
        }
    }

    @Override
    protected void onDestroy() {
        android.util.Log.i("**my", "configuration destroy()");
        super.onDestroy();
    }

    @Override
    public void onClick(View v) {
        String text;
        Toast toast;

        text = m_edit.getText().toString();
        if(DeviceService.getInstance().writeString(text, m_type) < 0){
            android.util.Log.e("**my", "write text failed: " + m_type);
            toast = Toast.makeText(getApplicationContext(), "Write file failed.", Toast.LENGTH_LONG);
            toast.show();
            return;
        }

        android.util.Log.i("**my", "save: " + text);
        toast = Toast.makeText(getApplicationContext(), "File Saved.", Toast.LENGTH_LONG);
        toast.show();
        return;
    }
}