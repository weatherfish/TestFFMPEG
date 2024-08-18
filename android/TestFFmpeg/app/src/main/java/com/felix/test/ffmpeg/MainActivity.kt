package com.felix.test.ffmpeg

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import com.felix.test.ffmpeg.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)

        setContentView(binding.root)

        binding.buttonPlay.setOnClickListener {
            val videoPath = "/sdcard/1.mp4"
            binding.videoView.playVideo(videoPath)
        }
    }

    companion object {
        init {
            System.loadLibrary("ffmpeg")
        }
    }
}