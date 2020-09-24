package com.app.vplayer

import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView

class VPlayer : SurfaceHolder.Callback {

    init {
        System.loadLibrary("native-lib")
    }

    external fun native_prepare(dataSource: String);
    external fun native_start();
    external fun native_set_surface(surface: Surface)

    private var dataSource: String = ""
    private var surfaceHolder: SurfaceHolder? = null

    // 初始化路径
    fun setDataSource(path: String) {
        this.dataSource = path
    }

    // 准备
    fun prepare() {
        this.native_prepare(dataSource)
    }

    fun start() {
        this.native_start()
    }

    fun setSurfaceView(surfaceView: SurfaceView) {
        this.surfaceHolder?.removeCallback(this)
        this.surfaceHolder = surfaceView.holder
        this.surfaceHolder?.addCallback(this)
    }

    override fun surfaceCreated(holder: SurfaceHolder?) {
        holder?.surface?.let { this.native_set_surface(it) }
    }

    override fun surfaceChanged(holder: SurfaceHolder?, format: Int, width: Int, height: Int) {
    }

    override fun surfaceDestroyed(holder: SurfaceHolder?) {
    }

    private var onPrepareListener: OnPrepareListener? = null
    private var onProgressListener: OnProgressListener? = null
    private var onErrorListener: OnErrorListener? = null

    fun setOnPrepareListener(onPrepareListener: OnPrepareListener) {
        this.onPrepareListener = onPrepareListener
    }

    fun setOnProgressListener(onProgressListener: OnProgressListener) {
        this.onProgressListener = onProgressListener;
    }

    fun setOnErrorListener(onErrorListener: OnErrorListener) {
        this.onErrorListener = onErrorListener;
    }


    fun onPrepare() {
        onPrepareListener?.onPrepared()
    }

    fun onProgress(progress: Int) {
        onProgressListener?.onProgress(progress)
    }

    fun onError(errorCode: Int) {
        onErrorListener?.onError(errorCode)
    }

    interface OnPrepareListener {
        fun onPrepared()
    }

    interface OnProgressListener {
        fun onProgress(progress: Int)
    }

    interface OnErrorListener {
        fun onError(errorCode: Int)
    }


}