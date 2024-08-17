package com.felix.test.ffmpeg.widget

import android.content.Context
import android.graphics.PixelFormat
import android.util.AttributeSet
import android.util.Log
import android.view.Surface
import android.view.SurfaceView
import com.felix.test.ffmpeg.utils.FFUtils

//
//  Created by wangqiang on 2024/8/17.
//  TestFFmpeg
//
//  FFVideoView

class FFVideoView : SurfaceView {
    private var mSurface: Surface? = null

    constructor(context: Context) : super(context) {
        init()
    }

    constructor(context: Context, attrs: AttributeSet) : super(context, attrs) {
        init()
    }

    constructor(context: Context, attrs: AttributeSet, defStyle: Int) : super(context, attrs, defStyle) {
        init()
    }

    private fun init() {
        holder.setFormat(PixelFormat.RGBA_8888)
        mSurface = holder.surface
    }

    fun playVideo(videoPath: String) {
        Thread(kotlinx.coroutines.Runnable {
            Log.d("FFVideoView", "run:: play video $videoPath")
            mSurface?.let { FFUtils.playVideo(videoPath, it) }
        }).start()
    }
}