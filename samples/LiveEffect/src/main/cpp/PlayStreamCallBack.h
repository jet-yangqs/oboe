//
// Created by noelyang on 2021/11/26.
//

#ifndef SAMPLES_PLAYSTREAMCALLBACK_H
#define SAMPLES_PLAYSTREAMCALLBACK_H

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "oboe/Oboe.h"
//#include "AudioEngine.h"

class AudioEngine;
class PlayStreamCallBack : public oboe::AudioStreamCallback {
public:
    PlayStreamCallBack() {}
    virtual ~PlayStreamCallBack() {}

    void setOutputStream(std::shared_ptr<oboe::AudioStream> stream) {
        mOutputStream = stream;
    }

    void setAudioEngine(AudioEngine* engine) {
        mAudioEngine = engine;
    }

    virtual oboe::Result start();

    virtual oboe::Result stop();


    /*
     * oboe::AudioStreamDataCallback interface implementation
     */
    oboe::DataCallbackResult onAudioReady( oboe::AudioStream *audioStream,
                                            void *audioData, int numFrames) override;

    /*
     * oboe::AudioStreamErrorCallback interface implementation
     */
    void onErrorBeforeClose(oboe::AudioStream *oboeStream, oboe::Result error) override;
    void onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) override;

    int32_t getNumInputBurstsCushion() const;

private:

    // let input fill back up, usually 0 or 1
    int32_t              mNumInputBurstsCushion = 1;

    std::shared_ptr<oboe::AudioStream> mOutputStream = nullptr;
    AudioEngine* mAudioEngine = nullptr;

};

#endif //SAMPLES_PLAYSTREAMCALLBACK_H
