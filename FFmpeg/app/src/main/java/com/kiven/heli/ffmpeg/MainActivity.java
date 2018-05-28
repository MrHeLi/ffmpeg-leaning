package com.kiven.heli.ffmpeg;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {
    private String TAG = this.getClass().getSimpleName();
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.sample_text);
        tv.setText(FFmpegPlayer.stringFromJNI());
        String sdcardPath = Environment.getExternalStorageDirectory().getPath();
        Log.i(TAG, "1");
        new Thread() {
            @Override
            public void run() {
                FFmpegPlayer.open("/storage/self/primary/Movies/output.mp4",this);
            }
        }.start();

//        FFmpegPlayer.open("/storage/self/primary/Movies/legao.flv",this);
    }
}
