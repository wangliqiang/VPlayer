//
// Created by wangl on 2020/9/22.
//
#include "VideoChannel.h"
#include "JavaCallHelper.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}
void dropFrame(queue<AVFrame *> &q) {
    if (!q.empty()) {
        AVFrame *frame = q.front();
        q.pop();
        BaseChannel::releaseAvFrame(frame);
    }
}
VideoChannel::VideoChannel(int id, JavaCallHelper *javaCallHelper, AVCodecContext *avCodecContext,
                           AVRational time_base)
        : BaseChannel(id, javaCallHelper, avCodecContext, time_base) {
    this->javaCallHelper = javaCallHelper;
    this->avCodecContext = avCodecContext;
    frame_queue.setReleaseHandle(releaseAvFrame);
    frame_queue.setSyncHandle(dropFrame);
}

void *decode(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->decodePacket();
    return 0;
}

void *synchronize(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->synchronizeFrame();
    return 0;
}


void VideoChannel::play() {
    pkt_queue.setWork(1);
    frame_queue.setWork(1);
    isPlaying = true;
    pthread_create(&pid_video_play, NULL, decode, this);
    pthread_create(&pid_synchronize, NULL, synchronize, this);
}

void VideoChannel::stop() {

}

void VideoChannel::decodePacket() {
    AVPacket *packet = 0;
    while (isPlaying) {
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
            //需要更多数据
            continue;
        } else if (ret < 0) {
            //失败  直播  端
            break;
        }
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, frame);
        frame_queue.enQueue(frame);
        while (frame_queue.size() > 100 && isPlaying) {
            av_usleep(1000 * 10);
            continue;
        }
    }
    releaseAvPacket(packet);

}

void VideoChannel::synchronizeFrame() {
    SwsContext *swsContext = sws_getContext(
            avCodecContext->width, avCodecContext->height, avCodecContext->pix_fmt,
            avCodecContext->width, avCodecContext->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, 0, 0, 0);

    uint8_t *dst_data[4];//argb
    int dst_linesize[4];
    av_image_alloc(dst_data, dst_linesize,
                   avCodecContext->width, avCodecContext->coded_height, AV_PIX_FMT_RGBA, 1);

    AVFrame *frame = 0;

    while (isPlaying) {
        int ret = frame_queue.deQueue(frame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        sws_scale(swsContext, reinterpret_cast<const uint8_t *const *>(frame->data),
                  frame->linesize, 0,
                  frame->height, dst_data, dst_linesize);

//        frame->pts;

        // 回调出去
        renderFrame(dst_data[0], dst_linesize[0], avCodecContext->width, avCodecContext->height);

        // 16ms
        LOGE("解码一帧视频 %d", frame_queue.size());
        // pts
        clock = frame->pts * av_q2d(time_base);
        // 解码时间算进去
        double frame_delays = 1.0 / fps;
        double audioClock = audioChannel->clock;
        // 将解码所需要的的时间算进去，因为配置差的手机，解码需要耗费更多的时间
        double extra_delay = frame->repeat_pict / (2 * fps);
        double delay = extra_delay + frame_delays;

        double diff = clock - audioClock;
        LOGE("----相差------- %d",diff);
        // 视频超前
        if (clock > audioClock) {
            if (diff > 1) {
                av_usleep((delay * 2) * 1000000);
            } else {
                av_usleep((delay + diff) * 1000000);
            }
        } else {
            // 视频延后
            if (diff > 1) {
                // 不处理
            } else if (diff >= 0.05) {
                // 视频需要追赶 丢帧
                // 执行同步操作 删除到最近的key frame
                releaseAvFrame(frame);
                frame_queue.sync();
            } else {

            }
        }
        releaseAvFrame(frame);

    }
    av_freep(&dst_data[0]);
    isPlaying = false;
    releaseAvFrame(frame);
    sws_freeContext(swsContext);
}

void VideoChannel::setRenderCallback(RenderFrame renderFrame) {
    this->renderFrame = renderFrame;
}

void VideoChannel::setFps(int fps) {
    this->fps = fps;
}

