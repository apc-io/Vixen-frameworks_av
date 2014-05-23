/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef CORE_MEDIA_BUFFER_H_
#define CORE_MEDIA_BUFFER_H_

#include <pthread.h>

#include <utils/Errors.h>
#include <utils/RefBase.h>

namespace android {

class CoreMediaBuffer;
class CoreMediaBufferObserver;
//class MetaData;

class CoreMediaBufferObserver {
public:
    CoreMediaBufferObserver() {}
    virtual ~CoreMediaBufferObserver() {}

    virtual void signalBufferReturned(CoreMediaBuffer *buffer) = 0;

private:
    CoreMediaBufferObserver(const CoreMediaBufferObserver &);
    CoreMediaBufferObserver &operator=(const CoreMediaBufferObserver &);
};

class CoreMediaBuffer {
public:
    // The underlying data remains the responsibility of the caller!
    CoreMediaBuffer(void *data, size_t size);

    CoreMediaBuffer(size_t size);

    // Decrements the reference count and returns the buffer to its
    // associated MediaBufferGroup if the reference count drops to 0.
    void release();

    // Increments the reference count.
    void add_ref();

    void *data() const;
    size_t size() const;

    size_t range_offset() const;
    size_t range_length() const;

    void set_range(size_t offset, size_t length);

    //sp<MetaData> meta_data();
    int64_t getTime();
    void setTime(int64_t value);

    // Clears meta data and resets the range to the full extent.
    void reset();

    void setObserver(CoreMediaBufferObserver *group);

    // Returns a clone of this CoreMediaBuffer increasing its reference count.
    // The clone references the same data but has its own range and
    // MetaData.
    CoreMediaBuffer *clone();

    int refcount() const;

protected:
    virtual ~CoreMediaBuffer();

private:
    friend class CoreMediaBufferGroup;
    //friend class OMXDecoder;

    // For use by OMXDecoder, reference count must be 1, drop reference
    // count to 0 without signalling the observer.
    void claim();

    CoreMediaBufferObserver *mObserver;
    CoreMediaBuffer *mNextBuffer;
    int mRefCount;

    void *mData;
    size_t mSize, mRangeOffset, mRangeLength;

    bool mOwnsData;

    //sp<MetaData> mMetaData;
    int64_t mTime;

    CoreMediaBuffer *mOriginal;

    void setNextBuffer(CoreMediaBuffer *buffer);
    CoreMediaBuffer *nextBuffer();

    CoreMediaBuffer(const CoreMediaBuffer &);
    CoreMediaBuffer &operator=(const CoreMediaBuffer &);
};

}  // namespace android

#endif  // CoreMEDIA_BUFFER_H_
