//
// Created by wangl on 2020/9/22.
//

#ifndef VPLAYER_AUDIOCHANNEL_H
#define VPLAYER_AUDIOCHANNEL_H

#include <SLES/OpenSLES_Android.h>
#include "BaseChannel.h"

extern "C" {
#include <libswresample/swresample.h>
};

class AudioChannel : public BaseChannel {
public:
    AudioChannel(int id, JavaCallHelper *javaCallHelper, AVCodecContext *avCodecContext,
                 AVRational time_base
    );

    virtual void play();

    virtual void stop();

    void initOpenSL();

    void decode();

    int getPcm();

private:
    pthread_t pid_audio_play;
    pthread_t pid_audio_decode;
    SwrContext *swrContext = NULL;
    int out_channels;
    int out_samplesize;
    int out_sample_rate;
public:
    uint8_t *buffer;
};


#endif //VPLAYER_AUDIOCHANNEL_H
