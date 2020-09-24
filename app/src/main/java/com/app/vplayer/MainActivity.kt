package com.app.vplayer

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.view.WindowManager
import android.widget.SeekBar
import androidx.appcompat.app.AppCompatActivity
import kotlinx.android.synthetic.main.activity_main.*
import java.io.File


class MainActivity : AppCompatActivity(), SeekBar.OnSeekBarChangeListener {

    private var seekPosition: Int = 0
    private var totalDuration: Int = 0
    private lateinit var vPlayer: VPlayer

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setFlags(
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
        )
        setContentView(R.layout.activity_main)
        seekBar.setOnSeekBarChangeListener(this)

        checkPermission()

        vPlayer = VPlayer()
        vPlayer.setSurfaceView(surfaceView)

        val file = File(Environment.getExternalStorageDirectory(), "input.mp4")
        vPlayer.setDataSource(file.absolutePath)

        vPlayer.setOnPrepareListener(object : VPlayer.OnPrepareListener {
            override fun onPrepared() {
                vPlayer.start()
            }
        })

        vPlayer.setOnProgressListener(object : VPlayer.OnProgressListener {
            override fun onProgress(progress: Int) {
                runOnUiThread {
                    current_time.text = formatTime(progress)
                    val position: Int = progress * 100 / 115
                    seekBar.progress = position
                }
            }

        })
        vPlayer.getDisplayDuration(object : VPlayer.Duration {
            override fun get(duration: Int) {
                runOnUiThread {
                    totalDuration = duration;
                    total_duration.text = formatTime(totalDuration)
                }
            }

        })
        play.setOnClickListener {
            vPlayer.prepare()
        }

    }

    override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
        seekPosition = progress;
        vPlayer.seekTo(seekPosition * totalDuration / 100)
    }

    override fun onStartTrackingTouch(seekBar: SeekBar?) {
    }

    override fun onStopTrackingTouch(seekBar: SeekBar?) {
    }

    private fun checkPermission() {
        var isGranted = true
        if (Build.VERSION.SDK_INT >= 23) {
            if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                //如果没有写sd卡权限
                isGranted = false
            }
            if (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                isGranted = false
            }
            Log.i("cbs", "isGranted == $isGranted")
            if (!isGranted) {
                requestPermissions(
                    arrayOf(
                        Manifest.permission.ACCESS_COARSE_LOCATION,
                        Manifest.permission.ACCESS_FINE_LOCATION,
                        Manifest.permission.READ_EXTERNAL_STORAGE,
                        Manifest.permission.WRITE_EXTERNAL_STORAGE
                    ),
                    102
                )
            }
        }
    }

    private fun formatTime(time: Int): String? {
        val minute = time / 60
        val second = time % 60
        return (if (minute < 10) "0$minute" else minute).toString() + ":" + if (second < 10) "0$second" else second
    }
}
