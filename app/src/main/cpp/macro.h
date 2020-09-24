//
// Created by wangl on 2020/9/22.
//

#ifndef VPLAYER_MACRO_H
#define VPLAYER_MACRO_H

#include <android/log.h>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"FFMPEG",__VA_ARGS__)
#define DELETE(obj) if(obj) { delete obj; obj = 0; }

// 标记线程，子线程需要attach
#define THREAD_MAIN 1
#define THREAD_CHILD 2

// 错误代码
// 打不开视频
#define FFMPEG_CAN_NOT_OPEN_URL 1
// 找不到流媒体
#define FFMPEG_CAN_NOT_FIND_STREAMS 2
// 找不到解码器
#define FFMPEG_FIND_DECODER_FAIL 3
// 无法根据解码器创建上下文
#define FFMPEG_ALLOC_CODEC_CONTEXT_FAIL 4
// 根据流信息配置上下文失败
#define FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL 5
// 打开解码器失败
#define FFMPEG_OPEN_DECODER_FAIL 6
// 没有音视频
#define FFMPEG_NO_MEDIA  7

#endif //VPLAYER_MACRO_H
