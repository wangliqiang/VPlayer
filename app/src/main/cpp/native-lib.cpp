#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include "JavaCallHelper.h"
#include "VPlayerFFmpeg.h"
extern  "C"{
#include "libavcodec/avcodec.h"
}

JavaCallHelper *javaCallHelper;
ANativeWindow *window = 0;
VPlayerFFmpeg *vPlayerFFmpeg;

JavaVM *javaVm = NULL;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVm = vm;
    return JNI_VERSION_1_4;
}

// 渲染
void renderFrame(uint8_t *data, int linesize, int w, int h) {
    // 设置窗口属性
    ANativeWindow_setBuffersGeometry(window, w, h, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;
        return;
    }
    // 缓冲区 window_data[0] = 255
    uint8_t *window_data = static_cast<uint8_t *>(window_buffer.bits);
    // R G B A int*4
    int window_linesize = window_buffer.stride * 4;
    uint8_t *src_data = data;
    for (int i = 0; i < window_buffer.height; ++i) {
        memcpy(window_data + i * window_linesize, src_data + i * linesize, window_linesize);
    }
    ANativeWindow_unlockAndPost(window);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_app_vplayer_VPlayer_native_1prepare(JNIEnv *env, jobject thiz, jstring dataSource_) {
    const char *dataSource = env->GetStringUTFChars(dataSource_, 0);
    javaCallHelper = new JavaCallHelper(javaVm, env, thiz);
    vPlayerFFmpeg = new VPlayerFFmpeg(javaCallHelper, dataSource);
    vPlayerFFmpeg->setRenderCallback(renderFrame);
    vPlayerFFmpeg->prepare();
    env->ReleaseStringUTFChars(dataSource_, dataSource);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_app_vplayer_VPlayer_native_1start(JNIEnv *env, jobject thiz) {
    if (vPlayerFFmpeg) {
        vPlayerFFmpeg->start();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_app_vplayer_VPlayer_native_1set_1surface(JNIEnv *env, jobject thiz, jobject surface) {

    // 释放window
    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    // 创建新的窗口用于视频显示
    window = ANativeWindow_fromSurface(env, surface);
}extern "C"
JNIEXPORT void JNICALL
Java_com_app_vplayer_VPlayer_native_1seekTo(JNIEnv *env, jobject thiz, jint progress) {
    if(vPlayerFFmpeg){
        vPlayerFFmpeg->seekTo(progress);
    }
}