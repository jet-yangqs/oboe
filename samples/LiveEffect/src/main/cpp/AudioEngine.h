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


// These must match order in strings.xml and in StreamConfiguration.java
#define NATIVE_MODE_UNSPECIFIED  0
#define NATIVE_MODE_OPENSLES     1
#define NATIVE_MODE_AAUDIO       2

#define MAX_SINE_OSCILLATORS     8
#define AMPLITUDE_SINE           1.0
#define AMPLITUDE_SAWTOOTH       0.5
#define FREQUENCY_SAW_PING       800.0
#define AMPLITUDE_SAW_PING       0.8
#define AMPLITUDE_IMPULSE        0.7

#define NANOS_PER_MICROSECOND    ((int64_t) 1000)
#define NANOS_PER_MILLISECOND    (1000 * NANOS_PER_MICROSECOND)
#define NANOS_PER_SECOND         (1000 * NANOS_PER_MILLISECOND)

#define SECONDS_TO_RECORD        10

enum class nativeParamsResult : int32_t {
    OK = 0,
    ErrorBase = -1,
};

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
    int setupEngineConfigParameters(int peroidLenInMilliSeconds,
                                   void* p_record_bb,
                                   void* p_play_bb,
                                   int nativeApi,
                                   int sampleRate,
                                   int channelCount,
                                   int format,
                                   int sharingMode,
                                   int performanceMode,
                                   int inputPreset,
                                   int usage,
                                   int recordDeviceId,
                                   int playDeviceId,
                                   int sessionId,
                                   bool channelConversionAllowed,
                                   bool formatConversionAllowed,
                                   int rateConversionQuality
                                );
    int startAudio();
    int stopAudio();
    //void setupConfigParameters(int32_t sampleRate, int32_t peroidLenInMilliSeconds);
    //void setByteBufferAddress(void *p_record_bb, void *p_play_bb);

private:
    RecordStreamCallBack    mRecordStreamCallBack;
    PlayStreamCallBack      mPlayStreamCallBack;
    JniCallBack             mJniCallBack;
    bool              mIsEffectOn = false;
    bool              mAudioEngineOn = false;
    // builder parameters
    int32_t           mRecordingDeviceId = oboe::kUnspecified; //0
    int32_t           mPlaybackDeviceId = oboe::kUnspecified;
    const oboe::AudioFormat mFormat = oboe::AudioFormat::I16;
    const oboe::SharingMode mSharingMode = oboe::SharingMode::Shared;
    const oboe::PerformanceMode mPerformanceMode = oboe::PerformanceMode::LowLatency;
    const oboe::InputPreset mInputPreset = oboe::InputPreset::VoiceCommunication;
    const oboe::Usage mUsage = oboe::Usage::VoiceCommunication;
    oboe::AudioApi    mAudioApi = oboe::AudioApi::AAudio;
    oboe::SessionId mSessionId = oboe::SessionId::None; //-1
    const bool mChannelConversionAllowed = true;
    const bool mFormatConversionAllowed = true;
    oboe::SampleRateConversionQuality mSampleRateConversionQuality = oboe::SampleRateConversionQuality::Medium;
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
