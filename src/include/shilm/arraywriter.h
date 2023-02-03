#pragma once
#include "writer.h"
#include "buffer.h"
#include <vector>

namespace shilm {
namespace io {

class ArrayWriter : public BinaryWriter {
public:
    ArrayWriter(int endianness, int bitorder) :
        BinaryWriter(endianness, bitorder),
		mBuffer(1 << 15) {
        mData.resize(32768);
		mCapacity = 32768;
    }
    inline virtual void write(uint8_t b) override {
		mBuffer.add(b, (BinaryWriter&) *this);
    }
	inline virtual size_t write(const char* src, size_t size) override {
		mBuffer.add((unsigned char*) src, (unsigned int) size, *this);
		return size;
	}
    inline virtual void flushStream() override {
		if  (mSize + mBuffer.size() > mCapacity) {
			while (mSize + mBuffer.size() > mCapacity) {
				mCapacity *= 2;
			}
			mData.resize(mCapacity);
		}
		std::memcpy(&mData[0] + mSize, &mBuffer[0], mBuffer.size());
		mSize += mBuffer.size();
		mBuffer.reset();
    }
    inline std::vector<uint8_t> getData() {
		flushStream();
		mData.resize(mSize);
		mCapacity = mSize;
        return mData;
    }
private:
    std::vector<uint8_t> mData;
	unsigned int mSize = 0;
	unsigned int mCapacity = 0;
	Buffer mBuffer;
};

}
}

