/*
 * Copyright 2015 The Android Open Source Project
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

#ifndef FIFO_FIFO_BUFFER_H
#define FIFO_FIFO_BUFFER_H

#include <memory>
#include <stdint.h>

#include "FifoControllerBase.h"

namespace android {

/**
 * Structure that represents a region in a circular buffer that might be at the
 * end of the array and split in two.
 */
struct WrappingBuffer {
    enum {
        SIZE = 2
    };
    void *data[SIZE];
    int32_t numFrames[SIZE];
};

class FifoBuffer {
public:
    explicit FifoBuffer(int32_t bytesPerFrame);

    virtual ~FifoBuffer() = default;

    int32_t convertFramesToBytes(fifo_frames_t frames);

    fifo_frames_t read(void *destination, fifo_frames_t framesToRead);

    fifo_frames_t write(const void *source, fifo_frames_t framesToWrite);

    fifo_frames_t getThreshold();

    void setThreshold(fifo_frames_t threshold);

    fifo_frames_t getBufferCapacityInFrames();

    /**
     * Return pointer to available full frames in data1 and set size in numFrames1.
     * if the data is split across the end of the FIFO then set data2 and numFrames2.
     * Other wise set them to null
     * @param wrappingBuffer
     * @return total full frames available
     */
    fifo_frames_t getFullDataAvailable(WrappingBuffer *wrappingBuffer);

    /**
     * Return pointer to available empty frames in data1 and set size in numFrames1.
     * if the room is split across the end of the FIFO then set data2 and numFrames2.
     * Other wise set them to null
     * @param wrappingBuffer
     * @return total empty frames available
     */
    fifo_frames_t getEmptyRoomAvailable(WrappingBuffer *wrappingBuffer);

    int32_t getBytesPerFrame() {
        return mBytesPerFrame;
    }

    // Proxy methods for the internal FifoController

    fifo_counter_t getReadCounter() {
        return mFifo->getReadCounter();
    }

    void setReadCounter(fifo_counter_t n) {
        mFifo->setReadCounter(n);
    }

    fifo_counter_t getWriteCounter() {
        return mFifo->getWriteCounter();
    }

    void setWriteCounter(fifo_counter_t n) {
        mFifo->setWriteCounter(n);
    }

    void advanceReadIndex(fifo_frames_t numFrames) {
        mFifo->advanceReadIndex(numFrames);
    }

    void advanceWriteIndex(fifo_frames_t numFrames) {
        mFifo->advanceWriteIndex(numFrames);
    }

    fifo_frames_t getEmptyFramesAvailable() {
        return mFifo->getEmptyFramesAvailable();
    }

    fifo_frames_t getFullFramesAvailable() {
        return mFifo->getFullFramesAvailable();
    }

    /*
     * This is generally only called before or after the buffer is used.
     */
    void eraseMemory();

    /**
     * Clear some memory after the write pointer.
     * This can be used to prevent the reader from accidentally reading stale data
     * in case it is reading asynchronously.
     */
    fifo_frames_t eraseEmptyMemory(fifo_frames_t numFrames);

protected:

    virtual uint8_t *getStorage() const = 0;

    void fillWrappingBuffer(WrappingBuffer *wrappingBuffer,
                            int32_t framesAvailable, int32_t startIndex);

    const int32_t             mBytesPerFrame;
    std::unique_ptr<FifoControllerBase> mFifo{};
};

// Define two subclasses to handle the two ways that storage is allocated.

// Allocate storage internally.
class FifoBufferAllocated : public FifoBuffer {
public:
    FifoBufferAllocated(int32_t bytesPerFrame, fifo_frames_t capacityInFrames);

private:

    uint8_t *getStorage() const override {
        return mInternalStorage.get();
    };

    std::unique_ptr<uint8_t[]> mInternalStorage;
};

// Allocate storage externally and pass it in.
class FifoBufferIndirect : public FifoBuffer {
public:
    // We use raw pointers because the memory may be
    // in the middle of an allocated block and cannot be deleted directly.
    FifoBufferIndirect(int32_t bytesPerFrame,
                       fifo_frames_t capacityInFrames,
                       fifo_counter_t* readCounterAddress,
                       fifo_counter_t* writeCounterAddress,
                       void* dataStorageAddress);

private:

    uint8_t *getStorage() const override {
        return mExternalStorage;
    };

    uint8_t *mExternalStorage = nullptr;
};

}  // namespace android

#endif //FIFO_FIFO_BUFFER_H
