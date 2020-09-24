//
// Created by wangl on 2020/9/22.
//

#ifndef VPLAYER_VPLAYERFFMPEG_H
#define VPLAYER_VPLAYERFFMPEG_H

#include <pthread.h>
#include <android/native_window.h>
#include "JavaCallHelper.h"
#include <pthread.h>
#include <android/native_window.h>
#include "VideoChannel.h"
#include "AudioChannel.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
};

class VPlayerFFmpeg {
public:
    VPlayerFFmpeg(JavaCallHelper *javaCallHelper, const char *dataSource);

    ~VPlayerFFmpeg();

    void prepareFFmpeg();

    void prepare();

    void start();

    void play();

    void setRenderCallback(RenderFrame renderFrame);

private:
    bool isPlaying;
    char *url;
    pthread_t pid_prepare;// 销毁
    pthread_t pid_play; // 直到播放完毕
    VideoChannel *videoChannel;
    AudioChannel *audioChannel;
    AVFormatContext *formatContext;
    JavaCallHelper *javaCallHelper;
    RenderFrame renderFrame;
};


#endif //VPLAYER_VPLAYERFFMPEG_H
