package com.google.oboe.samples.liveEffect;
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

import android.content.Context;
import android.media.AudioManager;
import android.os.Build;
import android.util.Log;

import java.nio.ByteBuffer;

public class LiveEffectEngine {

    final static String TAG = "LiveEffectEngine.java: ";
    // Load native library
    static {
        System.loadLibrary("liveEffect");
    }

    //direct buffer
    // 16kHz * 20ms * 16pits_per_sample/8 * 1 channel= 640 inByte
    public static ByteBuffer byteBufferForRecord = ByteBuffer.allocateDirect(640);
    public static ByteBuffer byteBufferForPlay = ByteBuffer.allocateDirect(640);
    public static ByteBuffer javaInternalBuffer = ByteBuffer.allocateDirect(640);

    public final static int copyByteBuffer(final ByteBuffer from, ByteBuffer to) {
        final int len = Math.min(from.limit(), to.limit());
        System.arraycopy(from.array(), 0, to.array(), 0, len);
        return len;
    }

    void javaOnRecord(){
        Log.d("java layer: ","javaOnRecord: void func called from record back thread.");
        // copy from record buffer to engine buffer
        //Log.d(TAG, "javaOnRecord->byteBufferForRecord value: " + new String(byteBufferForRecord.array()) );
        int bytesCopied = copyByteBuffer(byteBufferForRecord, javaInternalBuffer);
        //Log.d(TAG, "javaOnRecord->javaInternalBuffer value: " + new String(javaInternalBuffer.array()) );

        assert(bytesCopied==640);
    }

    static void javaOnPlay(){
        Log.d("java layer: ", "javaOnPlay: static void func called from play back thread.");
        // copy from engine buffer to play buffer
        //Log.d(TAG, "javaOnPlay->byteBufferForPlay pre value: " + new String(byteBufferForPlay.array()) );
        int bytesCopied = copyByteBuffer(javaInternalBuffer, byteBufferForPlay);
        //Log.d(TAG, "javaOnPlay->byteBufferForPlay after value: " + new String(byteBufferForPlay.array()) );
        assert(bytesCopied==640);
    }

    // Native methods
    static native boolean create();
    static native boolean isAAudioRecommended();
    static native boolean setAPI(int apiType);
    static native boolean setEffectOn(boolean isEffectOn, ByteBuffer record_bb, ByteBuffer play_bb);
    static native void setRecordingDeviceId(int deviceId);
    static native void setPlaybackDeviceId(int deviceId);
    static native void delete();
    static native void native_setDefaultStreamValues(int defaultSampleRate, int defaultFramesPerBurst);


    static void setDefaultStreamValues(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1){
            AudioManager myAudioMgr = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
            String sampleRateStr = myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
            int defaultSampleRate = Integer.parseInt(sampleRateStr);
            String framesPerBurstStr = myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
            int defaultFramesPerBurst = Integer.parseInt(framesPerBurstStr);

            native_setDefaultStreamValues(defaultSampleRate, defaultFramesPerBurst);
        }
    }
}
