//
// Created by noelyang on 2021/11/26.
//
#include <logging_macros.h>
#include "RecordStreamCallBack.h"
#include "AudioEngine.h"

/**
 * Oboe notifies the application for "about to close the stream".
 *
 * @param oboeStream: the stream to close
 * @param error: oboe's reason for closing the stream
 */
void RecordStreamCallBack::onErrorBeforeClose(oboe::AudioStream *oboeStream,
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
void RecordStreamCallBack::onErrorAfterClose(oboe::AudioStream *oboeStream,
                                           oboe::Result error) {
    assert(mAudioEngine);
    //因为插入耳机record和play stream都关闭了，但是回调只在此处触发，play中没有触发，因此两个stream都要关闭重启
    mAudioEngine->handleDeviceChange();
    LOGE("%s stream Error after close: %s",
         oboe::convertToText(oboeStream->getDirection()),
         oboe::convertToText(error));
}

oboe::DataCallbackResult RecordStreamCallBack::onAudioReady(
        oboe::AudioStream *inputStream,
        void *audioData,
        int numFrames) {

    oboe::DataCallbackResult callbackResult = oboe::DataCallbackResult::Continue;
    // copy data from engine buffer to output stream
    assert(mAudioEngine);
    int32_t framesCopied = mAudioEngine->onRecord(inputStream, audioData, numFrames);
    assert(framesCopied>0);

    if (callbackResult == oboe::DataCallbackResult::Stop) {
        mInputStream->requestStop();
    }

    return callbackResult;
}

oboe::Result RecordStreamCallBack::start() {

    // assumes the data format for both streams is Int16.
    // assert(mInputStream->getFormat() == oboe::AudioFormat::I16);

    oboe::Result result = oboe::Result::ErrorBase;
    if(mInputStream){
        result = mInputStream->requestStart();
    }
    return result;
}

oboe::Result RecordStreamCallBack::stop() {
    oboe::Result result = oboe::Result::OK;
    if (mInputStream) {
        result = mInputStream->requestStop();
    }
    return result;
}

int32_t RecordStreamCallBack::getNumInputBurstsCushion() const {
    return mNumInputBurstsCushion;
}
