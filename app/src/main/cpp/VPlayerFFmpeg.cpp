//
// Created by wangl on 2020/9/22.
//
#include "VPlayerFFmpeg.h"
#include "JavaCallHelper.h"
#include "macro.h"

void *prepareFFmpeg_(void *args) {
    VPlayerFFmpeg *vPlayerFFmpeg = static_cast<VPlayerFFmpeg *>(args);
    vPlayerFFmpeg->prepareFFmpeg();
    return 0;
}

VPlayerFFmpeg::VPlayerFFmpeg(JavaCallHelper *javaCallHelper, const char *dataSource) {
    url = new char[strlen(dataSource) + 1];
    this->javaCallHelper = javaCallHelper;
    strcpy(url, dataSource);
}

VPlayerFFmpeg::~VPlayerFFmpeg() {

}

void VPlayerFFmpeg::prepare() {
    pthread_create(&pid_prepare, NULL, prepareFFmpeg_, this);
}

void VPlayerFFmpeg::prepareFFmpeg() {
    /**
     * 子线程访问到对象的属性
     */
    avformat_network_init();
    formatContext = avformat_alloc_context();
    // 打开URL
    AVDictionary *opts = NULL;
    // 设置超时时间3秒
    av_dict_set(&opts, "timeout", "3000000", 0);
    // 强制指定AVFormatContext中的AVInputFormat的，这个参数一般情况下可以设置为NULL，
    // 这样FFmpeg可以自动检测AVInputFormat。
    // 输如文件的封装格式
    // av_find_input_format("avi"); //rtmp流
    int ret = avformat_open_input(&formatContext, url, NULL, &opts);
    if (ret != 0) {
        if (javaCallHelper)
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        return;
    }
    // 查找流
    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }

    for (int i = 0; i < formatContext->nb_streams; ++i) {
        AVCodecParameters *codecpar = formatContext->streams[i]->codecpar;
        AVStream *stream = formatContext->streams[i];
        // 找到解码器
        AVCodec *dec = avcodec_find_decoder(codecpar->codec_id);
        if (!dec) {
            if (javaCallHelper)
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            return;
        }
        // 创建解码上下文
        AVCodecContext *codecContext = avcodec_alloc_context3(dec);
        if (!codecContext) {
            if (javaCallHelper)
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            return;
        }
        // 复制参数
        if (avcodec_parameters_to_context(codecContext, codecpar) < 0) {
            if (javaCallHelper)
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            return;
        }
        // 打开解码器
        if (avcodec_open2(codecContext, dec, 0) != 0) {
            if (javaCallHelper)
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            return;
        }
        // 获取视频总时长
        if (formatContext->duration != AV_NOPTS_VALUE) {
            if (javaCallHelper)
                javaCallHelper->getDuration(THREAD_CHILD, static_cast<int>(stream->duration *
                                                                           av_q2d(stream->time_base)));
        }

        if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            // 音频
            audioChannel = new AudioChannel(i, javaCallHelper, codecContext, stream->time_base);
        } else if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            // 视频
            AVRational frame_rate = stream->avg_frame_rate;
            int fps = av_q2d(frame_rate);
            videoChannel = new VideoChannel(i, javaCallHelper, codecContext, stream->time_base);
            videoChannel->setRenderCallback(renderFrame);
            videoChannel->setFps(fps);
        }
    }

    if (!audioChannel && !videoChannel) {
        if (javaCallHelper)
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_NO_MEDIA);
        return;
    }
    videoChannel->audioChannel = audioChannel;
    if (javaCallHelper)
        javaCallHelper->onPrepare(THREAD_CHILD);

}

void *startThread(void *args) {
    VPlayerFFmpeg *fFmpeg = static_cast<VPlayerFFmpeg *>(args);
    fFmpeg->play();
    return 0;
}

void VPlayerFFmpeg::start() {
    isPlaying = true;
    if (audioChannel) {
        audioChannel->play();
    }
    if (videoChannel) {
        videoChannel->play();
    }
    pthread_create(&pid_play, NULL, startThread, this);
}

void VPlayerFFmpeg::play() {
    int ret = 0;
    while (isPlaying) {
        // 对读取速度进行限制，避免消费不及时导致的OOM
        if (audioChannel && audioChannel->pkt_queue.size() > 100) {
            av_usleep(1000 * 10);
            continue;
        }
        if (videoChannel && videoChannel->pkt_queue.size() > 100) {
            av_usleep(1000 * 10);
            continue;
        }

        // 读取包
        AVPacket *packet = av_packet_alloc();
        // 从媒体中读取音频，视频包
        ret = av_read_frame(formatContext, packet);

        if (ret == 0) {
            // 将数据包加入对列
            if (audioChannel && packet->stream_index == audioChannel->channelId) {
                audioChannel->pkt_queue.enQueue(packet);
            } else if (videoChannel && packet->stream_index == videoChannel->channelId) {
                videoChannel->pkt_queue.enQueue(packet);
            }
        } else if (ret == AVERROR_EOF) {
            // 读取完毕，但不一定播放完毕
            if (videoChannel->pkt_queue.empty() && videoChannel->frame_queue.empty() &&
                audioChannel->pkt_queue.empty() && audioChannel->frame_queue.empty()) {
                LOGE("播放完毕...");
                break;
            }
        } else {
            break;
        }
    }
    isPlaying = 0;
    audioChannel->stop();
    videoChannel->stop();
}

void VPlayerFFmpeg::setRenderCallback(RenderFrame renderFrame) {
    this->renderFrame = renderFrame;
}

void VPlayerFFmpeg::seekTo(int progress) {
//    videoChannel->pkt_queue.clear();
//    videoChannel->frame_queue.clear();
//    int result = av_seek_frame(formatContext, formatContext->streams[1]->index,
//                               (int64_t) (progress / av_q2d(formatContext->streams[1]->time_base)),
//                               AVSEEK_FLAG_BACKWARD);
//    if (result) {
//        LOGE("Player Error : Can not seek video to %d", progress);
//        return;
//    }
}


