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

#include <cassert>
#include <logging_macros.h>

#include "AudioEngine.h"

extern JavaVM *gs_jvm;

static oboe::AudioApi convertNativeApiToAudioApi(int nativeApi) {
    switch (nativeApi) {
        default:
        case NATIVE_MODE_UNSPECIFIED:
            return oboe::AudioApi::Unspecified;
        case NATIVE_MODE_AAUDIO:
            return oboe::AudioApi::AAudio;
        case NATIVE_MODE_OPENSLES:
            return oboe::AudioApi::OpenSLES;
    }
}

AudioEngine::AudioEngine() {
    assert(mOutputChannelCount == mInputChannelCount);
}

AudioEngine::~AudioEngine() {
    if(mEngineBuffer) {
        delete mEngineBuffer;
        mEngineBuffer = nullptr;
    }
    mRecordByteBuffer = nullptr;
    mPlayByteBuffer = nullptr;
}


void AudioEngine::setRecordingDeviceId(int32_t deviceId) {
    mRecordingDeviceId = deviceId;
}

void AudioEngine::setPlaybackDeviceId(int32_t deviceId) {
    mPlaybackDeviceId = deviceId;
}

bool AudioEngine::isAAudioRecommended() {
    return oboe::AudioStreamBuilder::isAAudioRecommended();
}

bool AudioEngine::setAudioApi(oboe::AudioApi api) {
    if (mIsEffectOn) return false;
    mAudioApi = api;
    return true;
}

int AudioEngine::startAudio() {
    oboe::Result hr = openStreams();
    if( hr != oboe::Result::OK){
        return (jint)hr;
    }
    hr = mRecordStreamCallBack.start();
    if( hr != oboe::Result::OK){
        return (jint)hr;
    }
    hr = mPlayStreamCallBack.start();
    if( hr != oboe::Result::OK){
        return (jint)hr;
    }
    mAudioEngineOn = true;
    return (jint)oboe::Result::OK;
}

int AudioEngine::stopAudio() {
    //oboe::Result hr = oboe::Result::OK;
    if(mAudioEngineOn) {
        mRecordStreamCallBack.stop();
        mPlayStreamCallBack.stop();
        closeStreams();
        mAudioEngineOn = false;
    }
    return 0;
}

int AudioEngine::startRecord() {
    oboe::Result hr = openRecordStream();
    if( hr != oboe::Result::OK){
        return (int)hr;
    }
    hr = mRecordStreamCallBack.start();
    if( hr != oboe::Result::OK){
        return (int)hr;
    }
    //mAudioEngineOn = true;
    return (int)hr;
}

int AudioEngine::startPlay() {
    oboe::Result hr = openPlayStream();
    if( hr != oboe::Result::OK){
        return (int)hr;
    }
    hr = mPlayStreamCallBack.start();
    if( hr != oboe::Result::OK){
        return (int)hr;
    }
    //mAudioEngineOn = true;
    return (int)hr;
}

int AudioEngine::stopRecord() {
    mRecordStreamCallBack.stop();
    closeStream(mRecordStream);
    mRecordStreamCallBack.setInputStream(nullptr);
    mRecordStreamCallBack.setAudioEngine(nullptr);
    return 0;
}

int AudioEngine::stopPlay() {
    mPlayStreamCallBack.stop();
    closeStream(mPlayStream);
    mPlayStreamCallBack.setOutputStream(nullptr);
    mPlayStreamCallBack.setAudioEngine(nullptr);
    return 0;
}

int AudioEngine::handleDeviceChange() {
    int hr = handleRecordDeviceChange();
    hr = handlePlayDeviceChange();
    return hr;
}

int AudioEngine::handleRecordDeviceChange() {
    stopRecord();
    int hr = startRecord();
    if(hr != 0){
        //todo: need notify UI ?
    }
    return hr;
}

int AudioEngine::handlePlayDeviceChange() {
    stopPlay();
    int hr = startPlay();
    if(hr != 0){
        //todo: need notify UI ?
    }
    return hr;
}

bool AudioEngine::setEffectOn(bool isOn) {
    bool success = true;
    if (isOn != mIsEffectOn) {
        if (isOn) {
            success = openStreams() == oboe::Result::OK;
            if (success) {
                mRecordStreamCallBack.start();
                mPlayStreamCallBack.start();
                mIsEffectOn = isOn;
            }
        } else {
            mRecordStreamCallBack.stop();
            mPlayStreamCallBack.stop();
            closeStreams();
            mIsEffectOn = isOn;
       }
    }
    return success;
}

void AudioEngine::closeStreams() {
    /*
    * Note: The order of events is important here.
    * The playback stream must be closed before the recording stream. If the
    * recording stream were to be closed first the playback stream's
    * callback may attempt to read from the recording stream
    * which would cause the app to crash since the recording stream would be
    * null.
    */
    closeStream(mPlayStream);
    mPlayStreamCallBack.setOutputStream(nullptr);
    mPlayStreamCallBack.setAudioEngine(nullptr);
    closeStream(mRecordStream);
    mRecordStreamCallBack.setInputStream(nullptr);
    mRecordStreamCallBack.setAudioEngine(nullptr);
    mBufferSizeInBytes = 0;
}

int AudioEngine::onPlay(oboe::AudioStream *outputStream, void *audioData, int numFrames){


    // const char *engineBuffer = static_cast<const char *>(mEngineBuffer);

    // 16kHz * 20ms * 16pits_per_sample/8 * 1 channel= 640 inByte
    int32_t bufferSizeInBytes = outputStream->getSampleRate() * mPeroidLenInMilliSeconds/1000 *
                                 2 * outputStream->getChannelCount();
    assert(bufferSizeInBytes == 640);
    assert(numFrames == 320);
    assert(bufferSizeInBytes <= mBufferSizeInBytes);
    assert(mEngineBuffer);

    std::lock_guard<std::mutex> lock(mBufferMutex);
    // 1. copy data from engineBuffer to outputBuffer
    // std::memcpy(audioData, engineBuffer, bufferSizeInBytes);

    // 2. copy data from java direct byte buffer to output buffer
    assert(mPlayByteBuffer);
    memcpy(audioData, mPlayByteBuffer, bufferSizeInBytes);

    mJniCallBack.onPlay((char*)audioData, numFrames);
    return numFrames;
}

int AudioEngine::onRecord(oboe::AudioStream *inputStream, void *audioData, int numFrames){

    // This code assumes the data format for both streams is int16.
    // const char *inputData = static_cast<const char *>(audioData);

    // 16kHz * 20ms * 16pits_per_sample/8 * 1 channel= 640 inByte
    int32_t bufferSizeInBytes = inputStream->getSampleRate() * mPeroidLenInMilliSeconds/1000 *
                                 2 * inputStream->getChannelCount();
    assert(bufferSizeInBytes == 640);
    assert(numFrames == 320);
    assert(bufferSizeInBytes <= mBufferSizeInBytes);
    assert(mEngineBuffer);


    std::lock_guard<std::mutex> lock(mBufferMutex);
    // 1. copy data from record stream to engineBuffer
    // std::memcpy(mEngineBuffer, inputData, bufferSizeInBytes);
    // 2. copy date from record  stream to java direct byte buffer
    assert(mRecordByteBuffer);
    memcpy(mRecordByteBuffer, audioData, bufferSizeInBytes);
    mJniCallBack.onRecord((char*)audioData, numFrames);
    return numFrames;
}

//void AudioEngine::setByteBufferAddress(void *p_record_bb, void *p_play_bb){
//    mRecordByteBuffer = (char*)p_record_bb;
//    mPlayByteBuffer = (char*)p_play_bb;
//}

oboe::Result  AudioEngine::openRecordStream() {
    // config record stream
    oboe::AudioStreamBuilder inBuilder;
    setupRecordingStreamParameters(&inBuilder);
    oboe::Result result = inBuilder.openStream(mRecordStream);
    if (result != oboe::Result::OK) {
        LOGE("Failed to open input stream. Error %s", oboe::convertToText(result));
        return result;
    }
    int32_t recordBufferCapacityInBytes = mRecordStream->getBufferCapacityInFrames()
                                              * mRecordStream->getChannelCount() * 2;
    // buffer size need less than buffer capacity
    assert(mBufferSizeInBytes <= recordBufferCapacityInBytes);

    warnIfNotLowLatency(mRecordStream);
    mRecordStreamCallBack.setInputStream(mRecordStream);
    mRecordStreamCallBack.setAudioEngine(this);
    return result;
}

oboe::Result AudioEngine::openPlayStream() {
    oboe::AudioStreamBuilder outBuilder;
    // config play stream
    setupPlaybackStreamParameters(&outBuilder);
    oboe::Result result = outBuilder.openStream(mPlayStream);
    if (result != oboe::Result::OK) {
        LOGE("Failed to open output stream. Error %s", oboe::convertToText(result));
        return result;
    }
    int32_t playBufferCapacityInBytes = mPlayStream->getBufferCapacityInFrames()
                                        * mPlayStream->getChannelCount() * 2;
    assert(mBufferSizeInBytes <= playBufferCapacityInBytes);
    warnIfNotLowLatency(mPlayStream);

    mPlayStreamCallBack.setOutputStream(mPlayStream);
    mPlayStreamCallBack.setAudioEngine(this);
    return result;
}

oboe::Result  AudioEngine::openStreams() {

    oboe::Result result = openRecordStream();
    if (result != oboe::Result::OK) {
        LOGE("Failed to open input stream. Error %s", oboe::convertToText(result));
        return result;
    }

    result = openPlayStream();
    if (result != oboe::Result::OK) {
        LOGE("Failed to open output stream. Error %s", oboe::convertToText(result));
        closeStream(mRecordStream);
        return result;
    }

    return result;
}

//void AudioEngine::setupConfigParameters(int32_t sampleRate, int32_t peroidLenInMilliSeconds){
//    mSampleRate = sampleRate;
//    mPeroidLenInMilliSeconds = peroidLenInMilliSeconds;
//    mBufferSizeInBytes = sampleRate * peroidLenInMilliSeconds / 1000 * 2;
//}

int AudioEngine::setupEngineConfigParameters(int peroidLenInMilliSeconds, void* p_record_bb, void* p_play_bb,
                               int nativeApi, int sampleRate, int channelCount, int format, int sharingMode,
                               int performanceMode, int inputPreset, int usage, int recordDeviceId, int playDeviceId,
                               int sessionId, bool channelConversionAllowed, bool formatConversionAllowed,
                               int rateConversionQuality){

    oboe::AudioApi audioApi = oboe::AudioApi::Unspecified;
    switch (nativeApi) {
        case NATIVE_MODE_UNSPECIFIED:
        case NATIVE_MODE_AAUDIO:
        case NATIVE_MODE_OPENSLES:
            audioApi = convertNativeApiToAudioApi(nativeApi);
            break;
        default:
            return (jint) oboe::Result::ErrorOutOfRange;
    }

    if (channelCount < 0 || channelCount > 256) {
        LOGE("ActivityContext::open() channels out of range");
        return (jint) oboe::Result::ErrorOutOfRange;
    }
    mPeroidLenInMilliSeconds = peroidLenInMilliSeconds;
    mRecordByteBuffer = static_cast<char *>(p_record_bb);
    mPlayByteBuffer = static_cast<char *> (p_play_bb);
    mAudioApi = audioApi;
    //assert(mAudioApi == oboe::AudioApi::AAudio);
    mSampleRate = sampleRate;
    assert(mSampleRate == 16000);
    if(mFormat == (oboe::AudioFormat)format){
        mBufferSizeInBytes = sampleRate * peroidLenInMilliSeconds / 1000 * 2;
        if(!mEngineBuffer) {
            mEngineBuffer = new char[mBufferSizeInBytes];
        }
    }
    else{
        assert(mFormat == (oboe::AudioFormat)format);
    }
    assert(mInputChannelCount == channelCount);
    assert(mOutputChannelCount == channelCount);
    assert(mSharingMode == (oboe::SharingMode)sharingMode);
    assert(mPerformanceMode == (oboe::PerformanceMode) performanceMode);
    assert(mInputPreset == (oboe::InputPreset)inputPreset);
    assert(mUsage == (oboe::Usage)usage);
    mRecordingDeviceId = recordDeviceId;
    mPlaybackDeviceId = playDeviceId;
    assert(mSessionId == (oboe::SessionId)sessionId);
    assert(mChannelConversionAllowed == channelConversionAllowed);
    assert(mFormatConversionAllowed == formatConversionAllowed);
    mSampleRateConversionQuality = (oboe::SampleRateConversionQuality)rateConversionQuality;

    return (jint)nativeParamsResult::OK;
}

/**
 * Sets the stream parameters which are specific to recording,
 * including the sample rate which is determined from the
 * playback stream.
 *
 * @param builder The recording stream builder
 * @param sampleRate The desired sample rate of the recording stream
 */
oboe::AudioStreamBuilder *AudioEngine::setupRecordingStreamParameters(
    oboe::AudioStreamBuilder *builder) {
    // This sample uses blocking read() because we don't specify a callback
    // 16kHz * 20Ms  = 320 frames //  * 1 channel = 320 samples // * 2 Bytes = 640 Bytes

    int32_t framesPerCallback = mSampleRate * mPeroidLenInMilliSeconds/1000;
    assert(framesPerCallback == 320);
    builder->setDataCallback(&mRecordStreamCallBack)
            ->setErrorCallback(&mRecordStreamCallBack)
            ->setDeviceId(mRecordingDeviceId)
            ->setDirection(oboe::Direction::Input)
            ->setSampleRate(mSampleRate)
            ->setInputPreset(mInputPreset)
            ->setChannelCount(mInputChannelCount)
            ->setFramesPerDataCallback(framesPerCallback)
            ->setBufferCapacityInFrames(framesPerCallback*2);
    // ->setInputPreset(oboe::InputPreset::VoicePerformance) // effect for live recording and playing
    return setupCommonStreamParameters(builder);
}

/**
 * Sets the stream parameters which are specific to playback, including device
 * id and the dataCallback function, which must be set for low latency
 * playback.
 * @param builder The playback stream builder
 */
oboe::AudioStreamBuilder *AudioEngine::setupPlaybackStreamParameters(
    oboe::AudioStreamBuilder *builder) {

    int32_t framesPerCallback = mSampleRate * mPeroidLenInMilliSeconds/1000;
    assert(framesPerCallback == 320);
    builder->setDataCallback(&mPlayStreamCallBack)
            ->setDeviceId(mPlaybackDeviceId)
            ->setDirection(oboe::Direction::Output)
            ->setSampleRate(mSampleRate)
            ->setChannelCount(mOutputChannelCount)
            ->setFramesPerDataCallback(framesPerCallback)
            ->setBufferCapacityInFrames(framesPerCallback*2);

    return setupCommonStreamParameters(builder);
}



/**
 * Set the stream parameters which are common to both recording and playback
 * streams.
 * @param builder The playback or recording stream builder
 */
oboe::AudioStreamBuilder *AudioEngine::setupCommonStreamParameters(
    oboe::AudioStreamBuilder *builder) {
    builder->setAudioApi(mAudioApi)
            ->setFormat(mFormat)
            ->setFormatConversionAllowed(mFormatConversionAllowed)
            ->setChannelConversionAllowed(mChannelConversionAllowed)
            ->setSharingMode(mSharingMode)
            ->setPerformanceMode(mPerformanceMode);

    return builder;
}

/**
 * Close the stream. AudioStream::close() is a blocking call so
 * the application does not need to add synchronization between
 * onAudioReady() function and the thread calling close().
 * [the closing thread is the UI thread in this sample].
 * @param stream the stream to close
 */
void AudioEngine::closeStream(std::shared_ptr<oboe::AudioStream> &stream) {
    if (stream) {
        oboe::Result result = stream->stop();
        if (result != oboe::Result::OK) {
            LOGW("Error stopping stream: %s", oboe::convertToText(result));
        }
        result = stream->close();
        if (result != oboe::Result::OK) {
            LOGE("Error closing stream: %s", oboe::convertToText(result));
        } else {
            LOGW("Successfully closed streams");
        }
        stream.reset();
    }
}

/**
 * Warn in logcat if non-low latency stream is created
 * @param stream: newly created stream
 *
 */
void AudioEngine::warnIfNotLowLatency(std::shared_ptr<oboe::AudioStream> &stream) {
    if (stream->getPerformanceMode() != oboe::PerformanceMode::LowLatency) {
        LOGW(
            "Stream is NOT low latency."
            "Check your requested format, sample rate and channel count");
    }
}


