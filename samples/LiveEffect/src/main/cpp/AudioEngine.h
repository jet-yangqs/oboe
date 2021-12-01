/*
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

#ifndef OBOE_AUDIOENGINE_H
#define OBOE_AUDIOENGINE_H

#include <jni.h>
#include <oboe/Oboe.h>
#include <string>
#include <thread>
#include "RecordStreamCallBack.h"
#include "PlayStreamCallBack.h"
#include "JniCallBack.h"
#include <mutex>

class AudioEngine {
public:
    AudioEngine();

    void setRecordingDeviceId(int32_t deviceId);
    void setPlaybackDeviceId(int32_t deviceId);
    int onPlay(oboe::AudioStream *outputStream, void *audioData, int numFrames);
    int onRecord(oboe::AudioStream *inputStream, void *audioData, int numFrames);
    /**
     * @param isOn
     * @return true if it succeeds
     */
    bool setEffectOn(bool isOn);
    bool setAudioApi(oboe::AudioApi);
    bool isAAudioRecommended(void);
    void setupConfigParameters(int32_t sampleRate, int32_t peroidLenInMilliSeconds);
    void setByteBufferAddress(void *p_record_bb, void *p_play_bb);

private:
    RecordStreamCallBack    mRecordStreamCallBack;
    PlayStreamCallBack      mPlayStreamCallBack;
    JniCallBack             mJniCallBack;
    bool              mIsEffectOn = false;
    // builder parameters
    int32_t           mRecordingDeviceId = oboe::kUnspecified;
    int32_t           mPlaybackDeviceId = oboe::kUnspecified;
    const oboe::AudioFormat mFormat = oboe::AudioFormat::I16;
    oboe::AudioApi    mAudioApi = oboe::AudioApi::AAudio;
    int32_t           mSampleRate = 16000;
    const int32_t     mInputChannelCount = oboe::ChannelCount::Mono;
    const int32_t     mOutputChannelCount = oboe::ChannelCount::Mono;

    // buffer info
    int32_t              mBufferSizeInBytes = 0;
    int32_t              mPeroidLenInMilliSeconds  = 20;
    char *               mEngineBuffer = nullptr;
    char *               mRecordByteBuffer = nullptr;
    char *               mPlayByteBuffer = nullptr;
    std::mutex           mBufferMutex;

    std::shared_ptr<oboe::AudioStream> mRecordingStream;
    std::shared_ptr<oboe::AudioStream> mPlayStream;

    oboe::Result openStreams();

    void closeStreams();

    void closeStream(std::shared_ptr<oboe::AudioStream> &stream);

    oboe::AudioStreamBuilder *setupCommonStreamParameters(
        oboe::AudioStreamBuilder *builder);
    oboe::AudioStreamBuilder *setupRecordingStreamParameters(
        oboe::AudioStreamBuilder *builder);
    oboe::AudioStreamBuilder *setupPlaybackStreamParameters(
        oboe::AudioStreamBuilder *builder);
    void warnIfNotLowLatency(std::shared_ptr<oboe::AudioStream> &stream);
};

#endif  // OBOE_AUDIOENGINE_H
