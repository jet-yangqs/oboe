//
// Created by noelyang on 2021/11/30.
//

#include "JniCallBack.h"
#include <logging_macros.h>

#define TAG "my_test"

Mutex g_lock;
extern JavaVM *gs_jvm;
extern jobject g_mjobject;

bool getJNIEnv(JNIEnv** env) {
    if (!gs_jvm) {
        *env = NULL;
        return false;
    }

    int checkEnv = gs_jvm->GetEnv((void**)env,JNI_VERSION_1_6);

    bool acttached = false;
    if (checkEnv < 0) {
        JavaVMAttachArgs attachArgs;
        attachArgs.version = JNI_VERSION_1_6;
        attachArgs.name = "default";
        attachArgs.group = NULL;

        int m_status = gs_jvm->AttachCurrentThread(env, &attachArgs);
        if (m_status < 0) {
            return false;
        }
        acttached = true;
    }

    return acttached;
}

void maybeDettachCurrThread(bool isAttached) {

    if (isAttached && gs_jvm) {
        gs_jvm->DetachCurrentThread();
    }
}

int JniCallBack::onPlay(char *audioData, int numFrames){
    // call back to java
    g_lock.tryLock();
    if (gs_jvm == NULL) {
        LOGE("(jvm is null)onBindNetChannel");
        g_lock.unlock();
        return -1;
    }
    LOGI("AudioEngine::onPlay c++ func in play thread");
    JNIEnv* env = NULL;
    bool attached = getJNIEnv(&env);
    //getJNIEnv(&env);

    if( env != NULL && g_mjobject!= NULL){
        //jclass mjclass = env->FindClass("com/google/oboe/samples/liveEffect/LiveEffectEngine");
        jmethodID mid = env->GetStaticMethodID((jclass)g_mjobject, "javaOnPlay", "()V");
        env->CallStaticVoidMethod((jclass)g_mjobject, mid);
    }
    maybeDettachCurrThread(attached);
    g_lock.unlock();
    return 0;
}


int JniCallBack::onRecord(char *audioData, int numFrames){
    // call back to java
    g_lock.tryLock();
    if (gs_jvm == NULL) {
        LOGE("(jvm is null)onBindNetChannel");
        g_lock.unlock();
        return -1;
    }
    LOGI("AudioEngine::onRecord c++ func in record thread");
    JNIEnv* env = NULL;
    bool attached = getJNIEnv(&env);
    //getJNIEnv(&env);

    if( env != NULL && g_mjobject != NULL){
        //jclass mjclass = env->FindClass("com/google/oboe/samples/liveEffect/LiveEffectEngine");
        jmethodID mid = env->GetMethodID((jclass)g_mjobject, "javaOnRecord", "()V");
        jobject mobj = env->AllocObject((jclass)g_mjobject);
        env->CallVoidMethod(mobj, mid);
    }
    maybeDettachCurrThread(attached);
    g_lock.unlock();
    return 0;
}