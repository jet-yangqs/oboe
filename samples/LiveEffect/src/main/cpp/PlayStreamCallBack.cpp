//
// Created by noelyang on 2021/11/26.
//
#include <logging_macros.h>
#include "PlayStreamCallBack.h"
#include "AudioEngine.h"

/**
 * Oboe notifies the application for "about to close the stream".
 *
 * @param oboeStream: the stream to close
 * @param error: oboe's reason for closing the stream
 */
void PlayStreamCallBack::onErrorBeforeClose(oboe::AudioStream *oboeStream,
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
void PlayStreamCallBack::onErrorAfterClose(oboe::AudioStream *oboeStream,
                                    oboe::Result error) {
    LOGE("%s stream Error after close: %s",
         oboe::convertToText(oboeStream->getDirection()),
         oboe::convertToText(error));
}


/**
 * Handles playback stream's audio request. In this codes, we simply block-read
 * from the record stream for the required samples.
 *
 * @param oboeStream: the playback stream that requesting additional samples
 * @param audioData:  the buffer to load audio samples for playback stream
 * @param numFrames:  number of frames to load to audioData buffer
 * @return: DataCallbackResult::Continue.
 */
oboe::DataCallbackResult PlayStreamCallBack::onAudioReady(
        oboe::AudioStream *outputStream,
        void *audioData,
        int numFrames) {

    oboe::DataCallbackResult callbackResult = oboe::DataCallbackResult::Continue;
    // copy data from engine buffer to output stream
    assert(mAudioEngine);
    int32_t framesCopied = mAudioEngine->onPlay(outputStream, audioData, numFrames);
    assert(framesCopied>0);

    if (callbackResult == oboe::DataCallbackResult::Stop) {
        mOutputStream->requestStop();
    }
    return callbackResult;
}


oboe::Result PlayStreamCallBack::start() {

    // assumes the data format for both streams is Int16.
    // assert(mOutputStream->getFormat() == oboe::AudioFormat::I16);
    oboe::Result result = oboe::Result::ErrorBase;
    if(mOutputStream){
        result = mOutputStream->requestStart();
    }
    return result;
}

oboe::Result PlayStreamCallBack::stop() {
    oboe::Result outputResult = oboe::Result::OK;
    if (mOutputStream) {
        outputResult = mOutputStream->requestStop();
    }
    return outputResult;
}


int32_t PlayStreamCallBack::getNumInputBurstsCushion() const {
    return mNumInputBurstsCushion;
}

