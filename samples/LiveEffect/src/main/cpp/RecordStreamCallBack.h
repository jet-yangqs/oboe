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
//#include "AudioEngine.h"

class AudioEngine;
class RecordStreamCallBack : public oboe::AudioStreamCallback {
public:
    RecordStreamCallBack() {}
    virtual ~RecordStreamCallBack() {
        mAudioEngine = nullptr;
    }

    void setInputStream(std::shared_ptr<oboe::AudioStream> stream) {
        mInputStream = stream;
    }

    void setAudioEngine(AudioEngine * engine) {
        mAudioEngine = engine;
    }

    virtual oboe::Result start();

    virtual oboe::Result stop();


    /**
     * Called by Oboe when the stream is ready to process audio.
     * This implements the stream synchronization. App should NOT override this method.
     */
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *audioStream,
                                            void *audioData, int numFrames) override;

    void onErrorBeforeClose(oboe::AudioStream *oboeStream, oboe::Result error) override;
    void onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) override;

    int32_t getNumInputBurstsCushion() const;


private:


    // let input fill back up, usually 0 or 1
    int32_t              mNumInputBurstsCushion = 1;


    std::shared_ptr<oboe::AudioStream> mInputStream = nullptr;
    AudioEngine* mAudioEngine = nullptr;
};

#endif //SAMPLES_RECORDSTREAMCALLBACK_H
