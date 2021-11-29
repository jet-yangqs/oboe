//
// Created by noelyang on 2021/11/26.
//

#include "RecordStreamCallBack.h"


oboe::DataCallbackResult RecordStreamCallBack::onAudioReady(
        oboe::AudioStream *inputStream,
        void *audioData,
        int numFrames) {
    // This code assumes the data format for both streams is int16.
    const char *inputData = static_cast<const char *>(audioData);

    oboe::DataCallbackResult callbackResult = oboe::DataCallbackResult::Continue;
    // int32_t actualFramesRead = 0;

    // Silence mInputBuffer.
    //int32_t numBytes = numFrames * outputStream->getBytesPerFrame();
    std::memset(mInputBuffer, 0 /* value */, mBufferSizeInBytes);

    // 16kHz * 20ms * 16pits_per_sample/8 * 1 channel= 640 inByte
    int32_t fragmentLenInBytes = inputStream->getSampleRate() * mPeroidLenInMilliSeconds/1000 *
    2 * inputStream->getChannelCount();
    assert(fragmentLenInBytes == 640);

    assert(fragmentLenInBytes == numFrames*2*1);
    // copy data from record stream to mInputBuffer
    std::memcpy(mInputBuffer, inputData, fragmentLenInBytes);

    // write date to output stream
    // assume same audio format with input stream

    oboe::ResultWithValue<int32_t> result = mOutputStream->write(mInputBuffer,
                                                                numFrames /* numFrames */,
                                                                0 /* timeoutNanoseconds */ );

    if (!result) {
        callbackResult = oboe::DataCallbackResult::Stop;
    } else {
        int32_t framesWritten = result.value();
        if(numFrames != framesWritten){
            //todo: log something
        }
    }

    if (callbackResult == oboe::DataCallbackResult::Stop) {
        mOutputStream->requestStop();
    }

    return callbackResult;
}


oboe::Result RecordStreamCallBack::start() {

    // assumes the data format for both streams is Int16.
    assert(mInputStream->getFormat() == oboe::AudioFormat::I16);

    // Determine maximum size that could possibly be called.
    int32_t bufferSizeInBytes = mInputStream->getBufferCapacityInFrames()
                         * mInputStream->getChannelCount() * 2;

    // assert(mBufferSizeInBytes <= bufferSizeInBytes);

    if (bufferSizeInBytes > mBufferSizeInBytes) {
        //malloc mInputBuffer memory space,
        mInputBuffer = new char[bufferSizeInBytes];
        mBufferSizeInBytes = bufferSizeInBytes;
    }

    oboe::Result result = mInputStream->requestStart();
    if (result != oboe::Result::OK) {
        return result;
    }
    return mOutputStream->requestStart();
}

oboe::Result RecordStreamCallBack::stop() {
    oboe::Result outputResult = oboe::Result::OK;
    oboe::Result inputResult = oboe::Result::OK;
    if (mOutputStream) {
        outputResult = mOutputStream->requestStop();
    }
    if (mInputStream) {
        inputResult = mInputStream->requestStop();
    }
    if (outputResult != oboe::Result::OK) {
        return outputResult;
    } else {
        return inputResult;
    }
}

void RecordStreamCallBack::unInit(){
    mBufferSizeInBytes = 0;
    delete mInputBuffer;
    mInputBuffer = nullptr;
}

int32_t RecordStreamCallBack::getNumInputBurstsCushion() const {
    return mNumInputBurstsCushion;
}

void RecordStreamCallBack::setNumInputBurstsCushion(int32_t numBursts) {
    RecordStreamCallBack::mNumInputBurstsCushion = numBursts;
}
