/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef AUDIORECORD_H_
#define AUDIORECORD_H_

#include <binder/IMemory.h>
#include <cutils/sched_policy.h>
#include <media/AudioSystem.h>
#include <media/IAudioRecord.h>
#include <system/audio.h>
#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <utils/threads.h>

namespace android {

class audio_track_cblk_t;

// ----------------------------------------------------------------------------

class AudioRecord : virtual public RefBase
{
public:

    static const int DEFAULT_SAMPLE_RATE = 48000;//44100; modify for hal tell audio flinger sr 48k to 
			                        //avoid redundant resample in hal

    /* Events used by AudioRecord callback function (callback_t).
     * Keep in sync with frameworks/base/media/java/android/media/AudioRecord.java NATIVE_EVENT_*.
     */
    enum event_type {
        EVENT_MORE_DATA = 0,        // Request to read more data from PCM buffer.
        EVENT_OVERRUN = 1,          // PCM buffer overrun occured.
        EVENT_MARKER = 2,           // Record head is at the specified marker position
                                    // (See setMarkerPosition()).
        EVENT_NEW_POS = 3,          // Record head is at a new position
                                    // (See setPositionUpdatePeriod()).
    };

    /* Create Buffer on the stack and pass it to obtainBuffer()
     * and releaseBuffer().
     */

    class Buffer
    {
    public:
        enum {
            MUTE    = 0x00000001
        };
        uint32_t    flags;
        int         channelCount;
        audio_format_t format;
        size_t      frameCount;
        size_t      size;           // total size in bytes == frameCount * frameSize
        union {
            void*       raw;
            short*      i16;
            int8_t*     i8;
        };
    };

    /* As a convenience, if a callback is supplied, a handler thread
     * is automatically created with the appropriate priority. This thread
     * invokes the callback when a new buffer becomes ready or an overrun condition occurs.
     * Parameters:
     *
     * event:   type of event notified (see enum AudioRecord::event_type).
     * user:    Pointer to context for use by the callback receiver.
     * info:    Pointer to optional parameter according to event type:
     *          - EVENT_MORE_DATA: pointer to AudioRecord::Buffer struct. The callback must not read
     *          more bytes than indicated by 'size' field and update 'size' if less bytes are
     *          read.
     *          - EVENT_OVERRUN: unused.
     *          - EVENT_MARKER: pointer to const uint32_t containing the marker position in frames.
     *          - EVENT_NEW_POS: pointer to const uint32_t containing the new position in frames.
     */

    typedef void (*callback_t)(int event, void* user, void *info);

    /* Returns the minimum frame count required for the successful creation of
     * an AudioRecord object.
     * Returned status (from utils/Errors.h) can be:
     *  - NO_ERROR: successful operation
     *  - NO_INIT: audio server or audio hardware not initialized
     *  - BAD_VALUE: unsupported configuration
     */

     static status_t getMinFrameCount(int* frameCount,
                                      uint32_t sampleRate,
                                      audio_format_t format,
                                      audio_channel_mask_t channelMask);

    /* Constructs an uninitialized AudioRecord. No connection with
     * AudioFlinger takes place.
     */
                        AudioRecord();

    /* Creates an AudioRecord track and registers it with AudioFlinger.
     * Once created, the track needs to be started before it can be used.
     * Unspecified values are set to the audio hardware's current
     * values.
     *
     * Parameters:
     *
     * inputSource:        Select the audio input to record to (e.g. AUDIO_SOURCE_DEFAULT).
     * sampleRate:         Track sampling rate in Hz.
     * format:             Audio format (e.g AUDIO_FORMAT_PCM_16_BIT for signed
     *                     16 bits per sample).
     * channelMask:        Channel mask.
     * frameCount:         Total size of track PCM buffer in frames. This defines the
     *                     latency of the track.
     * cbf:                Callback function. If not null, this function is called periodically
     *                     to provide new PCM data.
     * user:               Context for use by the callback receiver.
     * notificationFrames: The callback function is called each time notificationFrames PCM
     *                     frames are ready in record track output buffer.
     * sessionId:          Not yet supported.
     */

                        AudioRecord(audio_source_t inputSource,
                                    uint32_t sampleRate = 0,
                                    audio_format_t format = AUDIO_FORMAT_DEFAULT,
                                    audio_channel_mask_t channelMask = AUDIO_CHANNEL_IN_MONO,
                                    int frameCount      = 0,
                                    callback_t cbf = NULL,
                                    void* user = NULL,
                                    int notificationFrames = 0,
                                    int sessionId = 0);


    /* Terminates the AudioRecord and unregisters it from AudioFlinger.
     * Also destroys all resources associated with the AudioRecord.
     */
                        ~AudioRecord();


    /* Initialize an uninitialized AudioRecord.
     * Returned status (from utils/Errors.h) can be:
     *  - NO_ERROR: successful intialization
     *  - INVALID_OPERATION: AudioRecord is already intitialized or record device is already in use
     *  - BAD_VALUE: invalid parameter (channels, format, sampleRate...)
     *  - NO_INIT: audio server or audio hardware not initialized
     *  - PERMISSION_DENIED: recording is not allowed for the requesting process
     * */
            status_t    set(audio_source_t inputSource = AUDIO_SOURCE_DEFAULT,
                            uint32_t sampleRate = 0,
                            audio_format_t format = AUDIO_FORMAT_DEFAULT,
                            audio_channel_mask_t channelMask = AUDIO_CHANNEL_IN_MONO,
                            int frameCount      = 0,
                            callback_t cbf = NULL,
                            void* user = NULL,
                            int notificationFrames = 0,
                            bool threadCanCallJava = false,
                            int sessionId = 0);


    /* Result of constructing the AudioRecord. This must be checked
     * before using any AudioRecord API (except for set()), using
     * an uninitialized AudioRecord produces undefined results.
     * See set() method above for possible return codes.
     */
            status_t    initCheck() const;

    /* Returns this track's latency in milliseconds.
     * This includes the latency due to AudioRecord buffer size
     * and audio hardware driver.
     */
            uint32_t     latency() const;

   /* getters, see constructor and set() */

            audio_format_t format() const;
            int         channelCount() const;
            uint32_t    frameCount() const;
            size_t      frameSize() const;
            audio_source_t inputSource() const;


    /* After it's created the track is not active. Call start() to
     * make it active. If set, the callback will start being called.
     * if event is not AudioSystem::SYNC_EVENT_NONE, the capture start will be delayed until
     * the specified event occurs on the specified trigger session.
     */
            status_t    start(AudioSystem::sync_event_t event = AudioSystem::SYNC_EVENT_NONE,
                              int triggerSession = 0);

    /* Stop a track. If set, the callback will cease being called and
     * obtainBuffer returns STOPPED. Note that obtainBuffer() still works
     * and will fill up buffers until the pool is exhausted.
     */
            void        stop();
            bool        stopped() const;

    /* get sample rate for this record track
     */
            uint32_t    getSampleRate() const;

    /* Sets marker position. When record reaches the number of frames specified,
     * a callback with event type EVENT_MARKER is called. Calling setMarkerPosition
     * with marker == 0 cancels marker notification callback.
     * If the AudioRecord has been opened with no callback function associated,
     * the operation will fail.
     *
     * Parameters:
     *
     * marker:   marker position expressed in frames.
     *
     * Returned status (from utils/Errors.h) can be:
     *  - NO_ERROR: successful operation
     *  - INVALID_OPERATION: the AudioRecord has no callback installed.
     */
            status_t    setMarkerPosition(uint32_t marker);
            status_t    getMarkerPosition(uint32_t *marker) const;


    /* Sets position update period. Every time the number of frames specified has been recorded,
     * a callback with event type EVENT_NEW_POS is called.
     * Calling setPositionUpdatePeriod with updatePeriod == 0 cancels new position notification
     * callback.
     * If the AudioRecord has been opened with no callback function associated,
     * the operation will fail.
     *
     * Parameters:
     *
     * updatePeriod:  position update notification period expressed in frames.
     *
     * Returned status (from utils/Errors.h) can be:
     *  - NO_ERROR: successful operation
     *  - INVALID_OPERATION: the AudioRecord has no callback installed.
     */
            status_t    setPositionUpdatePeriod(uint32_t updatePeriod);
            status_t    getPositionUpdatePeriod(uint32_t *updatePeriod) const;


    /* Gets record head position. The position is the total number of frames
     * recorded since record start.
     *
     * Parameters:
     *
     *  position:  Address where to return record head position within AudioRecord buffer.
     *
     * Returned status (from utils/Errors.h) can be:
     *  - NO_ERROR: successful operation
     *  - BAD_VALUE:  position is NULL
     */
            status_t    getPosition(uint32_t *position) const;

    /* returns a handle on the audio input used by this AudioRecord.
     *
     * Parameters:
     *  none.
     *
     * Returned value:
     *  handle on audio hardware input
     */
            audio_io_handle_t    getInput() const;

    /* returns the audio session ID associated with this AudioRecord.
     *
     * Parameters:
     *  none.
     *
     * Returned value:
     *  AudioRecord session ID.
     */
            int    getSessionId() const;

    /* obtains a buffer of "frameCount" frames. The buffer must be
     * filled entirely. If the track is stopped, obtainBuffer() returns
     * STOPPED instead of NO_ERROR as long as there are buffers available,
     * at which point NO_MORE_BUFFERS is returned.
     * Buffers will be returned until the pool (buffercount())
     * is exhausted, at which point obtainBuffer() will either block
     * or return WOULD_BLOCK depending on the value of the "blocking"
     * parameter.
     */

        enum {
            NO_MORE_BUFFERS = 0x80000001,
            STOPPED = 1
        };

            status_t    obtainBuffer(Buffer* audioBuffer, int32_t waitCount);
            void        releaseBuffer(Buffer* audioBuffer);


    /* As a convenience we provide a read() interface to the audio buffer.
     * This is implemented on top of obtainBuffer/releaseBuffer.
     */
            ssize_t     read(void* buffer, size_t size);

    /* Return the amount of input frames lost in the audio driver since the last call of this
     * function.  Audio driver is expected to reset the value to 0 and restart counting upon
     * returning the current value by this function call.  Such loss typically occurs when the
     * user space process is blocked longer than the capacity of audio driver buffers.
     * Unit: the number of input audio frames
     */
            unsigned int  getInputFramesLost() const;

private:
    /* copying audio tracks is not allowed */
                        AudioRecord(const AudioRecord& other);
            AudioRecord& operator = (const AudioRecord& other);

    /* a small internal class to handle the callback */
    class AudioRecordThread : public Thread
    {
    public:
        AudioRecordThread(AudioRecord& receiver, bool bCanCallJava = false);

        // Do not call Thread::requestExitAndWait() without first calling requestExit().
        // Thread::requestExitAndWait() is not virtual, and the implementation doesn't do enough.
        virtual void        requestExit();

                void        pause();    // suspend thread from execution at next loop boundary
                void        resume();   // allow thread to execute, if not requested to exit

    private:
        friend class AudioRecord;
        virtual bool        threadLoop();
        AudioRecord& mReceiver;
        virtual ~AudioRecordThread();
        Mutex               mMyLock;    // Thread::mLock is private
        Condition           mMyCond;    // Thread::mThreadExitedCondition is private
        bool                mPaused;    // whether thread is currently paused
    };

            // body of AudioRecordThread::threadLoop()
            bool processAudioBuffer(const sp<AudioRecordThread>& thread);

            status_t openRecord_l(uint32_t sampleRate,
                                audio_format_t format,
                                audio_channel_mask_t channelMask,
                                int frameCount,
                                audio_io_handle_t input);
            audio_io_handle_t getInput_l();
            status_t restoreRecord_l(audio_track_cblk_t*& cblk);

    sp<AudioRecordThread>   mAudioRecordThread;
    mutable Mutex           mLock;

    bool                    mActive;            // protected by mLock

    // for client callback handler
    callback_t              mCbf;
    void*                   mUserData;

    // for notification APIs
    uint32_t                mNotificationFrames;
    uint32_t                mRemainingFrames;
    uint32_t                mMarkerPosition;    // in frames
    bool                    mMarkerReached;
    uint32_t                mNewPosition;       // in frames
    uint32_t                mUpdatePeriod;      // in ms

    // constant after constructor or set()
    uint32_t                mFrameCount;
    audio_format_t          mFormat;
    uint8_t                 mChannelCount;
    audio_source_t          mInputSource;
    status_t                mStatus;
    uint32_t                mLatency;
    audio_channel_mask_t    mChannelMask;
    audio_io_handle_t       mInput;                     // returned by AudioSystem::getInput()
    int                     mSessionId;

    // may be changed if IAudioRecord object is re-created
    sp<IAudioRecord>        mAudioRecord;
    sp<IMemory>             mCblkMemory;
    audio_track_cblk_t*     mCblk;

    int                     mPreviousPriority;          // before start()
    SchedPolicy             mPreviousSchedulingGroup;
};

}; // namespace android

#endif /*AUDIORECORD_H_*/
