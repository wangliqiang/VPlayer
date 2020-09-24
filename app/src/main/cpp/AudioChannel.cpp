//
// Created by wangl on 2020/9/22.
//

#include "AudioChannel.h"

extern "C" {
#include <libavutil/time.h>
}

void *audioPlay(void *args) {
    AudioChannel *audio = static_cast<AudioChannel *>(args);
    audio->initOpenSL();
    return 0;
}

void *audioDecode(void *args) {
    AudioChannel *audio = static_cast<AudioChannel *>(args);
    audio->decode();
    return 0;
}

void AudioChannel::play() {
    swrContext = swr_alloc_set_opts(
            0,
            AV_CH_LAYOUT_STEREO,
            AV_SAMPLE_FMT_S16, out_sample_rate,
            avCodecContext->channel_layout,
            avCodecContext->sample_fmt,
            avCodecContext->sample_rate,
            0,
            0);

    swr_init(swrContext);
    pkt_queue.setWork(1);
    frame_queue.setWork(1);
    isPlaying = true;

    // 创建初始化OpenSL ES的线程
    pthread_create(&pid_audio_play, NULL, audioPlay, this);

    // 创建初始化音频解码线程
    pthread_create(&pid_audio_decode, NULL, audioDecode, this);


}


AudioChannel::AudioChannel(int id, JavaCallHelper *javaCallHelper, AVCodecContext *avCodecContext,
                           AVRational time_base)
        : BaseChannel(id, javaCallHelper, avCodecContext, time_base) {
    // 根据布局获取声道数
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    out_samplesize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    out_sample_rate = 44100;
    // CD 音频标准
    // 44100 双声道 2字节 out_samplesize 16位 2个字节
    buffer = (uint8_t *) malloc(out_sample_rate * out_samplesize * out_channels);
}

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(context);
    //pcm数据   原始音频数据
    int datalen = audioChannel->getPcm();
    if (datalen > 0) {
        (*bq)->Enqueue(bq, audioChannel->buffer, datalen);
    }
}

void AudioChannel::stop() {

}

void AudioChannel::initOpenSL() {
    // 音频引擎
    SLEngineItf engineInterface = NULL;
    // 音频对象
    SLObjectItf engineObject = NULL;
    // 混音器
    SLObjectItf outputMixObject = NULL;
    // 播放器
    SLObjectItf bqPlayerObject = NULL;
    // 回调接口
    SLPlayItf bqPlayerInterface = NULL;
    // 缓冲对列
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue = NULL;

    // 1.初始化播放引擎
    SLresult sLresult;
    sLresult = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != sLresult) {
        return;
    }
    sLresult = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != sLresult) {
        return;
    }
    //音频接口  相当于SurfaceHodler
    sLresult = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
    if (SL_RESULT_SUCCESS != sLresult) {
        return;
    }

    // 2.初始化播放引擎
    sLresult = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0, 0, 0);
    // 初始化混音器outputMixObject
    sLresult = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != sLresult) {
        return;;
    }
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};

    // PCM数据
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM, // 播放器pcm格式的数据
            2, // 2个声道（立体声）
            SL_SAMPLINGRATE_44_1, // 44100hz的频率
            SL_PCMSAMPLEFORMAT_FIXED_16, // 位数 16位
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, // 立体声（前左前右）
            SL_BYTEORDER_LITTLEENDIAN // 小端模式
    };

    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};

    SLDataSink audioSink = {&outputMix, NULL};
    SLDataSource slDataSource = {&android_queue, &pcm};
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    (*engineInterface)->CreateAudioPlayer(
            engineInterface, // 播放引擎
            &bqPlayerObject, // 播放器
            &slDataSource, // 播放器参数 播放缓冲对列  播放格式
            &audioSink, // 播放缓冲区
            1, // 播放接口回调个数
            ids, // 设置播放对列ID
            req // 是否采用内置的播放对列
    );

    // 初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    // 得到接口后调用 获取Player接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerInterface);
    // 获得播放器接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue);

    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);

    //设置播放状态
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);
    bqPlayerCallback(bqPlayerBufferQueue, this);
    LOGE("手动调用播放 packet %d", this->pkt_queue.size());

}

void AudioChannel::decode() {
    AVPacket *packet = 0;
    while (isPlaying) {
        // 音频packet
        int ret = pkt_queue.deQueue(packet);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }

        ret = avcodec_send_packet(avCodecContext, packet);
        releaseAvPacket(packet);
        if (ret == AVERROR(EAGAIN)) {
            // 需要更多数据
            continue;
        } else if (ret < 0) {
            // 失败
            break;
        }
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, frame);
        if (ret == AVERROR(EAGAIN)) {
            // 需要更多数据
            continue;
        } else if (ret < 0) {
            break;
        }

        while (frame_queue.size() > 100 && isPlaying) {
            av_usleep(1000 * 10);
            continue;
        }
        frame_queue.enQueue(frame);
    }
}

int AudioChannel::getPcm() {
    AVFrame *frame = 0;
    int data_size = 0;
    while (isPlaying) {
        int ret = frame_queue.deQueue(frame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        uint64_t dst_nb_samples = av_rescale_rnd(
                swr_get_delay(swrContext, frame->sample_rate) + frame->nb_samples,
                out_sample_rate,
                frame->sample_rate,
                AV_ROUND_UP
        );
        // 转换，返回值为转换后的sample个数，buffer malloc (size)
        int nb = swr_convert(swrContext, &buffer, dst_nb_samples,
                             (const uint8_t **) frame->data, frame->nb_samples);
        //转换后数据，  buffer size 44100*2*2
        data_size = nb * out_channels * out_samplesize;

        clock = frame->pts * av_q2d(time_base);
        break;
    }
    releaseAvFrame(frame);
    return data_size;
}
