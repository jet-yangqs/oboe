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
                mIsEffectOn = isOn;
            }
        } else {
            mRecordStreamCallBack.stop();
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
    mRecordStreamCallBack.setOutputStream(nullptr);

    closeStream(mRecordingStream);
    mRecordStreamCallBack.setInputStream(nullptr);
}

oboe::Result  AudioEngine::openStreams() {
    // Note: The order of stream creation is important. We create the recording
    // stream first, then use properties from the recording stream
    // (e.g. sample rate) to create the playback stream. By matching the
    // properties we should get the lowest latency path
    int32_t sampleRate = 16000;
    int32_t peroidLenInMilliSeconds = 20;
    int32_t bufferSize = sampleRate * peroidLenInMilliSeconds / 1000 * 2;
    oboe::AudioStreamBuilder inBuilder, outBuilder;
    setupRecordingStreamParameters(&inBuilder, sampleRate, peroidLenInMilliSeconds);
    oboe::Result result = inBuilder.openStream(mRecordingStream);
    if (result != oboe::Result::OK) {
        LOGE("Failed to open input stream. Error %s", oboe::convertToText(result));
        mSampleRate = oboe::kUnspecified;
        return result;
    } else {
        // The input stream needs to run at the same sample rate as the output.
        mSampleRate = mRecordingStream->getSampleRate();
        assert(mSampleRate == sampleRate);
    }
    mRecordingStream->setBufferSizeInFrames(bufferSize);
    int32_t recordBufferSize = mRecordingStream->getBufferSizeInFrames();
    assert(recordBufferSize>0);
    warnIfNotLowLatency(mRecordingStream);


    setupPlaybackStreamParameters(&outBuilder);
    result = outBuilder.openStream(mPlayStream);
    if (result != oboe::Result::OK) {
        LOGE("Failed to open output stream. Error %s", oboe::convertToText(result));
        closeStream(mRecordingStream);
        return result;
    }
    assert(mSampleRate == 16000);
    int32_t playSampleRate = mPlayStream->getSampleRate();
    assert(playSampleRate == mSampleRate);
    mPlayStream->setBufferSizeInFrames(bufferSize);
    int32_t playBufferSize = mPlayStream->getBufferSizeInFrames();
    assert(playBufferSize>0);
    warnIfNotLowLatency(mPlayStream);

    mRecordStreamCallBack.setInputStream(mRecordingStream);
    mRecordStreamCallBack.setOutputStream(mPlayStream);
    return result;
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
    oboe::AudioStreamBuilder *builder, int32_t sampleRate, int32_t peroidLenInMilliSeconds) {
    // This sample uses blocking read() because we don't specify a callback
    // 16kHz * 20Ms  = 320 frames //  * 1 channel = 320 samples // * 2 Bytes = 640 Bytes
    int32_t framesPerCallback = sampleRate * peroidLenInMilliSeconds/1000;
    assert(framesPerCallback == 320);
    builder->setDataCallback(this)
            ->setErrorCallback(this)
            ->setDeviceId(mRecordingDeviceId)
            ->setDirection(oboe::Direction::Input)
            ->setSampleRate(sampleRate)
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
    builder->setDeviceId(mPlaybackDeviceId)
            ->setDirection(oboe::Direction::Output)
            ->setSampleRate(mSampleRate)
            ->setChannelCount(mOutputChannelCount);

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

/**
 * Handles playback stream's audio request. In this sample, we simply block-read
 * from the record stream for the required samples.
 *
 * @param oboeStream: the playback stream that requesting additional samples
 * @param audioData:  the buffer to load audio samples for playback stream
 * @param numFrames:  number of frames to load to audioData buffer
 * @return: DataCallbackResult::Continue.
 */
oboe::DataCallbackResult AudioEngine::onAudioReady(
    oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) {
    return mRecordStreamCallBack.onAudioReady(oboeStream, audioData, numFrames);
}

/**
 * Oboe notifies the application for "about to close the stream".
 *
 * @param oboeStream: the stream to close
 * @param error: oboe's reason for closing the stream
 */
void AudioEngine::onErrorBeforeClose(oboe::AudioStream *oboeStream,
                                          oboe::Result error) {
    LOGE("%s stream Error before close: %s",
         oboe::convertToText(oboeStream->getDirection()),
         oboe::convertToText(error));
}

/**
 * Oboe notifies application that "the stream is closed"
 *
 * @param oboeStream
 * @param error
 */
void AudioEngine::onErrorAfterClose(oboe::AudioStream *oboeStream,
                                         oboe::Result error) {
    LOGE("%s stream Error after close: %s",
         oboe::convertToText(oboeStream->getDirection()),
         oboe::convertToText(error));
}
