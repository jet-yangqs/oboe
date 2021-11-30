//
// Created by noelyang on 2021/11/30.
//

#ifndef SAMPLES_JNICALLBACK_H
#define SAMPLES_JNICALLBACK_H
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <jni.h>

class JniCallBack {
    public:
        JniCallBack(){};
        int onPlay(char *audioData, int numFrames);
        int onRecord(char *audioData, int numFrames);
};


#endif // SAMPLES_JNICALLBACK_H


//----------------
class Mutex {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    Mutex();
    Mutex(const char* name);
    Mutex(int type, const char* name = NULL);
    ~Mutex();

    // lock or unlock the mutex
    int    lock();
    void        unlock();

    // lock if possible; returns 0 on success, error otherwise
    int    tryLock();

    // Manages the mutex automatically. It'll be locked when Autolock is
    // constructed and released when Autolock goes out of scope.
    class Autolock {
    public:
        inline Autolock(Mutex& mutex) : mLock(mutex)  { mLock.lock(); }
        inline Autolock(Mutex* mutex) : mLock(*mutex) { mLock.lock(); }
        inline ~Autolock() { mLock.unlock(); }
    private:
        Mutex& mLock;
    };

private:
    friend class Condition;

    // A mutex cannot be copied
    Mutex(const Mutex&);
    Mutex&      operator = (const Mutex&);

    pthread_mutex_t mMutex;

};


inline Mutex::Mutex() {
    pthread_mutex_init(&mMutex, NULL);
}
inline Mutex::Mutex(const char* name) {
    pthread_mutex_init(&mMutex, NULL);
}
inline Mutex::Mutex(int type, const char* name) {
    if (type == SHARED) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&mMutex, &attr);
        pthread_mutexattr_destroy(&attr);
    } else {
        pthread_mutex_init(&mMutex, NULL);
    }
}
inline Mutex::~Mutex() {
    pthread_mutex_destroy(&mMutex);
}
inline int Mutex::lock() {
    return -pthread_mutex_lock(&mMutex);
}
inline void Mutex::unlock() {
    pthread_mutex_unlock(&mMutex);
}
inline int Mutex::tryLock() {
    return -pthread_mutex_trylock(&mMutex);
}

typedef Mutex::Autolock AutoMutex;

//------------------------------------------------------------------------------
template<class T>
class JAuto
{
public:
    explicit JAuto(JNIEnv* env, T jptr) :
            mEnv(env), mJPtr(jptr)
    {

    }

    ~JAuto()
    {
        if (mEnv != NULL)
        {
            mEnv->DeleteLocalRef(mJPtr);
        }
    }

    operator T()
    {
        return mJPtr;
    }

    bool operator==(const T jptr) const
    {
        return mJPtr == jptr;
    }

    T Detach()
    {
        T ret = mJPtr;
        mJPtr = NULL;
        return ret;
    }

private:
    T mJPtr;
    JNIEnv* mEnv;
};