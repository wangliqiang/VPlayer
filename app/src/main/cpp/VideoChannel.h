//
// Created by wangl on 2020/9/22.
//

#ifndef VPLAYER_VIDEOCHANNEL_H
#define VPLAYER_VIDEOCHANNEL_H

#include "AudioChannel.h"
#include "BaseChannel.h"

extern "C" {
#include <libavcodec/avcodec.h>
};

typedef void (*RenderFrame)(uint8_t *, int, int, int);

class VideoChannel : public BaseChannel {
public:
    VideoChannel(int id, JavaCallHelper *javaCallHelper, AVCodecContext *avCodecContext,
                 AVRational time_base);

    virtual void play();

    virtual void stop();

    void decodePacket();

    void synchronizeFrame();

    void setRenderCallback(RenderFrame renderFrame);

    void setFps(int fps);

private:
    pthread_t pid_video_play;
    pthread_t pid_synchronize;
    RenderFrame renderFrame;
    int fps;
public:
    AudioChannel *audioChannel;
};


#endif //VPLAYER_VIDEOCHANNEL_H
