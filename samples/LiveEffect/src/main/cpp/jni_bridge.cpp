/**
 * Copyright 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <jni.h>
#include <logging_macros.h>
#include "AudioEngine.h"
// #include "aaudio/AAudioExtensions.h"


static const int kOboeApiAAudio = 0;
static const int kOboeApiOpenSLES = 1;

static AudioEngine *engine = nullptr;

JavaVM *gs_jvm = NULL;
jobject g_mjobject;



extern "C" {


JNIEXPORT jboolean JNICALL
Java_com_google_oboe_samples_liveEffect_OboeJavaAudioEngine_create(JNIEnv *env,
                                                               jclass jengine) {
    if (engine == nullptr) {
        engine = new AudioEngine();
    }

    env->GetJavaVM(&gs_jvm);
    g_mjobject = env->NewGlobalRef(jengine);
    return (engine != nullptr) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_google_oboe_samples_liveEffect_OboeJavaAudioEngine_createNativeAudioEngine(JNIEnv *env,
                                                                   jclass jengine) {
    if (engine == nullptr) {
        engine = new AudioEngine();
    }

    env->GetJavaVM(&gs_jvm);
    g_mjobject = env->NewGlobalRef(jengine);
    return (engine != nullptr) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_google_oboe_samples_liveEffect_OboeJavaAudioEngine_delete(JNIEnv *env,
                                                               jclass) {
    if (engine) {
        engine->setEffectOn(false);
        delete engine;
        engine = nullptr;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_google_oboe_samples_liveEffect_OboeJavaAudioEngine_setEffectOn(
    JNIEnv *env, jclass, jboolean isEffectOn, jobject record_bb, jobject play_bb) {
    if (engine == nullptr) {
        LOGE(
            "Engine is null, you must call createEngine before calling this "
            "method");
        return JNI_FALSE;
    }
    //void* p_record_bb = env->GetDirectBufferAddress(record_bb);
    //void* p_play_bb = env->GetDirectBufferAddress(play_bb);
    //engine->setByteBufferAddress(p_record_bb, p_play_bb);
    return engine->setEffectOn(isEffectOn) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_google_oboe_samples_liveEffect_OboeJavaAudioEngine_setupNativeAudioEngineParameters(
        JNIEnv *env, jclass,
        jint peroidLenInMilliSeconds,
        jobject record_bb,
        jobject play_bb,
        jint nativeApi,
        jint sampleRate,
        jint channelCount,
        jint format,
        jint sharingMode,
        jint performanceMode,
        jint inputPreset,
        jint usage,
        jint recordDeviceId,
        jint playDeviceId,
        jint sessionId,
        jboolean channelConversionAllowed,
        jboolean formatConversionAllowed,
        jint rateConversionQuality
        ){
            if (engine == nullptr) {
                LOGE(
                        "Engine is null, you must call createEngine before calling this "
                        "method");
                return -1;
            }

            void* p_record_bb = env->GetDirectBufferAddress(record_bb);
            void* p_play_bb = env->GetDirectBufferAddress(play_bb);

            return engine->setupEngineConfigParameters( peroidLenInMilliSeconds,
                                                        p_record_bb,
                                                        p_play_bb,
                                                        nativeApi,
                                                        sampleRate,
                                                        channelCount,
                                                        format,
                                                        sharingMode,
                                                        performanceMode,
                                                        inputPreset,
                                                        usage,
                                                        recordDeviceId,
                                                        playDeviceId,
                                                        sessionId,
                                                        channelConversionAllowed,
                                                        formatConversionAllowed,
                                                        rateConversionQuality
                                                        );
}

JNIEXPORT jint JNICALL
Java_com_google_oboe_samples_liveEffect_OboeJavaAudioEngine_startNativeAudio(
        JNIEnv *env, jclass jEngine) {
    if (engine == nullptr) {
        LOGE(
                "Engine is null, you must call createEngine before calling this "
                "method");
        return (jint)oboe::Result::ErrorBase;
    }
    return engine->startAudio();
}

JNIEXPORT jint JNICALL
Java_com_google_oboe_samples_liveEffect_OboeJavaAudioEngine_stopNativeAudio(
        JNIEnv *env, jclass jEngine) {
    if (engine == nullptr) {
        LOGE(
                "Engine is null, you must call createEngine before calling this "
                "method");
        return (jint)oboe::Result::ErrorBase;
    }
    return engine->stopAudio();
}

JNIEXPORT jint JNICALL
Java_com_google_oboe_samples_liveEffect_OboeJavaAudioEngine_releaseNativeAudioEngine(
        JNIEnv *env, jclass jEngine) {
    int hr = 0;
    if (engine) {
        hr = engine->stopAudio();
        delete engine;
        engine = nullptr;
        gs_jvm = NULL;
    }
    return hr;
}

JNIEXPORT void JNICALL
Java_com_google_oboe_samples_liveEffect_OboeJavaAudioEngine_setRecordingDeviceId(
    JNIEnv *env, jclass, jint deviceId) {
    if (engine == nullptr) {
        LOGE(
            "Engine is null, you must call createEngine before calling this "
            "method");
        return;
    }

    engine->setRecordingDeviceId(deviceId);
}

JNIEXPORT void JNICALL
Java_com_google_oboe_samples_liveEffect_OboeJavaAudioEngine_setPlaybackDeviceId(
    JNIEnv *env, jclass, jint deviceId) {
    if (engine == nullptr) {
        LOGE(
            "Engine is null, you must call createEngine before calling this "
            "method");
        return;
    }

    engine->setPlaybackDeviceId(deviceId);
}

JNIEXPORT jboolean JNICALL
Java_com_google_oboe_samples_liveEffect_OboeJavaAudioEngine_setAPI(JNIEnv *env,
                                                               jclass type,
                                                               jint apiType) {
    if (engine == nullptr) {
        LOGE(
            "Engine is null, you must call createEngine "
            "before calling this method");
        return JNI_FALSE;
    }

    oboe::AudioApi audioApi;
    switch (apiType) {
        case kOboeApiAAudio:
            audioApi = oboe::AudioApi::AAudio;
            break;
        case kOboeApiOpenSLES:
            audioApi = oboe::AudioApi::OpenSLES;
            break;
        default:
            LOGE("Unknown API selection to setAPI() %d", apiType);
            return JNI_FALSE;
    }

    return engine->setAudioApi(audioApi) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_google_oboe_samples_liveEffect_OboeJavaAudioEngine_isAAudioRecommended(
    JNIEnv *env, jclass type) {
    if (engine == nullptr) {
        LOGE(
            "Engine is null, you must call createEngine "
            "before calling this method");
        return JNI_FALSE;
    }
    return engine->isAAudioRecommended() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_google_oboe_samples_liveEffect_OboeJavaAudioEngine_native_1setDefaultStreamValues(JNIEnv *env,
                                               jclass type,
                                               jint sampleRate,
                                               jint framesPerBurst) {
    oboe::DefaultStreamValues::SampleRate = (int32_t) sampleRate;
    oboe::DefaultStreamValues::FramesPerBurst = (int32_t) framesPerBurst;
}


/**
 * for some AAudio test routines that are not part of the normal API.
 */
JNIEXPORT jboolean JNICALL
Java_com_google_oboe_samples_liveEffect_OboeJavaAudioEngine_isMMapSupported(
        JNIEnv *env, jclass type) {
    // return oboe::AAudioExtensions::getInstance().isMMapSupported();
    return false;
}

} // extern "C"
