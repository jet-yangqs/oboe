//
// Created by noelyang on 2021/11/26.
//

#ifndef SAMPLES_RECORDSTREAMCALLBACK_H
#define SAMPLES_RECORDSTREAMCALLBACK_H

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "oboe/Oboe.h"

class RecordStreamCallBack : public oboe::AudioStreamCallback {
public:
    RecordStreamCallBack() {}
    virtual ~RecordStreamCallBack() {
        unInit();
    }

    void setInputStream(std::shared_ptr<oboe::AudioStream> stream) {
        mInputStream = stream;
    }

    void setOutputStream(std::shared_ptr<oboe::AudioStream> stream) {
        mOutputStream = stream;
    }

    virtual oboe::Result start();

    virtual oboe::Result stop();


    /**
     * Called by Oboe when the stream is ready to process audio.
     * This implements the stream synchronization. App should NOT override this method.
     */
    oboe::DataCallbackResult onAudioReady(
            oboe::AudioStream *audioStream,
            void *audioData,
            int numFrames) override;

    int32_t getNumInputBurstsCushion() const;

    /**
     * Number of bursts to leave in the input buffer as a cushion.
     * Typically 0 for latency measurements
     * or 1 for glitch tests.
     *
     * @param mNumInputBurstsCushion
     */
    void setNumInputBurstsCushion(int32_t numInputBurstsCushion);
    void init();
    void unInit();
private:

    // TODO add getters and setters
    static constexpr int32_t kNumCallbacksToDrain   = 20;
    static constexpr int32_t kNumCallbacksToDiscard = 30;

    // let input fill back up, usually 0 or 1
    int32_t              mNumInputBurstsCushion = 1;

    // We want to reach a state where the input buffer is empty and
    // the output buffer is full.
    // These are used in order.
    // Drain several callback so that input is empty.
    int32_t              mCountCallbacksToDrain = kNumCallbacksToDrain;
    // Let the input fill back up slightly so we don't run dry.
    int32_t              mCountInputBurstsCushion = mNumInputBurstsCushion;
    // Discard some callbacks so the input and output reach equilibrium.
    int32_t              mCountCallbacksToDiscard = kNumCallbacksToDiscard;

    std::shared_ptr<oboe::AudioStream> mInputStream;
    std::shared_ptr<oboe::AudioStream> mOutputStream;

    int32_t              mBufferSizeInBytes = 0;
    char *               mInputBuffer = nullptr;
    int32_t              mPeroidLenInMilliSeconds  = 20;
};

#endif //SAMPLES_RECORDSTREAMCALLBACK_H
