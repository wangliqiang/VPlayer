//
// Created by wangl on 2020/9/22.
//

#ifndef VPLAYER_JAVACALLHELPER_H
#define VPLAYER_JAVACALLHELPER_H

#include <jni.h>

class JavaCallHelper {
public:
    JavaCallHelper(JavaVM *_javaVM, JNIEnv *_env, jobject &_jobj);

    ~JavaCallHelper();
    void onError(int thread, int code);

    void onPrepare(int thread);

    void onProgress(int thread, int progress);

private:
    JavaVM *javaVm;
    JNIEnv *env;
    jobject jobj;

    jmethodID jmid_prepare;
    jmethodID jmid_error;
    jmethodID jmid_progress;
};


#endif //VPLAYER_JAVACALLHELPER_H
