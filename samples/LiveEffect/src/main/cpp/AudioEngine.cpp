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

AudioEngine::AudioEngine() {
    assert(mOutputChannelCount == mInputChannelCount);
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
    closeStream(mRecordingStream);
    mRecordStreamCallBack.setInputStream(nullptr);
    mRecordStreamCallBack.setAudioEngine(nullptr);
    mBufferSizeInBytes = 0;
    if(mEngineBuffer){
        delete mEngineBuffer;
    }
}

int AudioEngine::onPlay(oboe::AudioStream *outputStream, void *audioData, int numFrames){


    const char *engineBuffer = static_cast<const char *>(mEngineBuffer);

    // 16kHz * 20ms * 16pits_per_sample/8 * 1 channel= 640 inByte
    int32_t bufferSizeInBytes = outputStream->getSampleRate() * mPeroidLenInMilliSeconds/1000 *
                                 2 * outputStream->getChannelCount();
    assert(bufferSizeInBytes == 640);
    assert(numFrames == 320);
    assert(bufferSizeInBytes <= mBufferSizeInBytes);
    assert(mEngineBuffer);
    // copy data from engineBuffer to outputBuffer
    std::lock_guard<std::mutex> lock(mBufferMutex);
    std::memcpy(audioData, engineBuffer, bufferSizeInBytes);
    mJniCallBack.onPlay((char*)audioData, numFrames);
    return numFrames;
}

int AudioEngine::onRecord(oboe::AudioStream *inputStream, void *audioData, int numFrames){

    // This code assumes the data format for both streams is int16.
    const char *inputData = static_cast<const char *>(audioData);

    // 16kHz * 20ms * 16pits_per_sample/8 * 1 channel= 640 inByte
    int32_t bufferSizeInBytes = inputStream->getSampleRate() * mPeroidLenInMilliSeconds/1000 *
                                 2 * inputStream->getChannelCount();
    assert(bufferSizeInBytes == 640);
    assert(numFrames == 320);
    assert(bufferSizeInBytes <= mBufferSizeInBytes);
    assert(mEngineBuffer);
    // copy data from record stream to engineBuffer

    std::lock_guard<std::mutex> lock(mBufferMutex);
    std::memcpy(mEngineBuffer, inputData, bufferSizeInBytes);
    mJniCallBack.onRecord((char*)audioData, numFrames);
    return numFrames;
}


oboe::Result  AudioEngine::openStreams() {

    int32_t sampleRate = 16000;
    int32_t peroidLenInMilliSeconds = 20;
    setupConfigParameters(sampleRate, peroidLenInMilliSeconds);

    // int32_t bufferSizeInBytes = sampleRate * peroidLenInMilliSeconds / 1000 * 2;

    // config record stream
    oboe::AudioStreamBuilder inBuilder, outBuilder;
    setupRecordingStreamParameters(&inBuilder);
    oboe::Result result = inBuilder.openStream(mRecordingStream);
    if (result != oboe::Result::OK) {
        LOGE("Failed to open input stream. Error %s", oboe::convertToText(result));
        return result;
    } else {
        int32_t recordBufferCapacityInBytes = mRecordingStream->getBufferCapacityInFrames()
                                    * mRecordingStream->getChannelCount() * 2;
        // buffer size need less than buffer capacity
        assert(mBufferSizeInBytes <= recordBufferCapacityInBytes);
        mPeroidLenInMilliSeconds = peroidLenInMilliSeconds;
        mEngineBuffer = new char[mBufferSizeInBytes];
    }

    // int32_t recordBufferSize = mRecordingStream->getBufferSizeInFrames();
    // assert(recordBufferSize>0);
    warnIfNotLowLatency(mRecordingStream);

    // config play stream
    setupPlaybackStreamParameters(&outBuilder);
    result = outBuilder.openStream(mPlayStream);
    if (result != oboe::Result::OK) {
        LOGE("Failed to open output stream. Error %s", oboe::convertToText(result));
        closeStream(mRecordingStream);
        return result;
    }
    int32_t playBufferCapacityInBytes = mPlayStream->getBufferCapacityInFrames()
                                * mPlayStream->getChannelCount() * 2;

    assert(mBufferSizeInBytes <= playBufferCapacityInBytes);
    warnIfNotLowLatency(mPlayStream);

    mRecordStreamCallBack.setInputStream(mRecordingStream);
    mRecordStreamCallBack.setAudioEngine(this);
    mPlayStreamCallBack.setOutputStream(mPlayStream);
    mPlayStreamCallBack.setAudioEngine(this);

    assert(mSampleRate == 16000);
    int32_t playSampleRate = mPlayStream->getSampleRate();
    assert(playSampleRate == mSampleRate);
    /*
     * mPlayStream->setBufferSizeInFrames(bufferSize);
     * int32_t playBufferSize = mPlayStream->getBufferSizeInFrames();
     * assert(playBufferSize>0);
    */
    return result;
}

void AudioEngine::setupConfigParameters(int32_t sampleRate, int32_t peroidLenInMilliSeconds){
    mSampleRate = sampleRate;
    mPeroidLenInMilliSeconds = peroidLenInMilliSeconds;
    mBufferSizeInBytes = sampleRate * peroidLenInMilliSeconds / 1000 * 2;
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
            ->setChannelCount(mInputChannelCount)
            ->setFramesPerDataCallback(framesPerCallback);
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
    assert(mSampleRate == 16000);
    int32_t framesPerCallback = mSampleRate * mPeroidLenInMilliSeconds/1000;
    assert(framesPerCallback == 320);
    builder->setDataCallback(&mPlayStreamCallBack)
            ->setDeviceId(mPlaybackDeviceId)
            ->setDirection(oboe::Direction::Output)
            ->setSampleRate(mSampleRate)
            ->setChannelCount(mOutputChannelCount)
            ->setFramesPerDataCallback(framesPerCallback);

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
            ->setFormatConversionAllowed(true)
            ->setChannelConversionAllowed(true)
            ->setSharingMode(oboe::SharingMode::Shared)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency);

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


